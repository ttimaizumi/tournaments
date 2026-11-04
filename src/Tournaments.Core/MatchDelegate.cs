namespace Tournaments.Core;

public sealed class MatchDelegate(
    ITournamentRepository tournaments,
    IGroupRepository groups,
    IMatchRepository matches)
{
    public async Task<IReadOnlyList<Match>> GetMatchesAsync(
        string tournamentId,
        MatchStage? stage = null,
        CancellationToken cancellationToken = default)
    {
        var all = await matches.GetAllAsync(tournamentId, cancellationToken);
        return stage is null ? all : all.Where(match => match.Stage == stage).ToList();
    }

    public Task<Match?> GetMatchAsync(string tournamentId, string matchId, CancellationToken cancellationToken = default) =>
        matches.GetAsync(tournamentId, matchId, cancellationToken);

    /// <summary>Round-robin table, or null when the tournament doesn't exist.</summary>
    public async Task<IReadOnlyList<Standing>?> GetStandingsAsync(
        string tournamentId,
        CancellationToken cancellationToken = default)
    {
        if (await tournaments.GetAsync(tournamentId, cancellationToken) is null)
        {
            return null;
        }

        var teams = (await groups.GetAllAsync(tournamentId, cancellationToken))
            .SelectMany(group => group.Teams)
            .ToList();
        var all = await matches.GetAllAsync(tournamentId, cancellationToken);
        return MatchScheduler.ComputeStandings(teams, all);
    }

    /// <summary>
    /// Creates the round-robin schedule once the group holds exactly the tournament's team count of
    /// unique teams. Safe to call repeatedly: returns 0 when the group isn't full or matches exist.
    /// </summary>
    public async Task<OperationResult<int>> GenerateRoundRobinAsync(
        string tournamentId,
        string groupId,
        CancellationToken cancellationToken = default)
    {
        var tournament = await tournaments.GetAsync(tournamentId, cancellationToken);
        if (tournament is null)
        {
            return OperationResult<int>.Failure("Tournament doesn't exist");
        }

        var group = await groups.GetAsync(tournamentId, groupId, cancellationToken);
        if (group is null)
        {
            return OperationResult<int>.Failure("Group doesn't exist");
        }

        if (group.Teams.Count < tournament.Format.MaxTeamsPerGroup)
        {
            return OperationResult<int>.Success(0);
        }

        var uniqueTeams = group.Teams.Select(team => team.Id).Distinct(StringComparer.Ordinal).Count();
        if (group.Teams.Count != tournament.Format.MaxTeamsPerGroup ||
            uniqueTeams != group.Teams.Count ||
            group.Teams.Any(team => string.IsNullOrEmpty(team.Id)))
        {
            return OperationResult<int>.Failure(
                $"Group {groupId} must hold exactly {tournament.Format.MaxTeamsPerGroup} unique teams");
        }

        var existing = await matches.GetAllAsync(tournamentId, cancellationToken);
        if (existing.Any(match => match.Stage == MatchStage.ROUND_ROBIN))
        {
            return OperationResult<int>.Success(0);
        }

        var fixtures = MatchScheduler.CreateRoundRobin(tournament.Id!, group.Id!, group.Teams);
        return OperationResult<int>.Success(await matches.CreateManyAsync(fixtures, cancellationToken));
    }

    /// <summary>
    /// Records a result, then schedules the playoffs when the round robin is complete or the next
    /// playoff round when the current one is.
    /// </summary>
    public async Task<OperationResult> UpdateScoreAsync(
        string tournamentId,
        string matchId,
        Score score,
        CancellationToken cancellationToken = default)
    {
        if (score.HomeTeamScore < 0 || score.VisitorTeamScore < 0)
        {
            return OperationResult.Failure("Scores can't be negative");
        }

        var match = await matches.GetAsync(tournamentId, matchId, cancellationToken);
        if (match is null)
        {
            return OperationResult.NotFound("Match doesn't exist");
        }

        var all = await matches.GetAllAsync(tournamentId, cancellationToken);
        var playoffs = all.Where(m => m.Stage == MatchStage.PLAYOFF).ToList();
        if (match.Stage == MatchStage.ROUND_ROBIN && playoffs.Count > 0)
        {
            return OperationResult.Failure("Round robin is closed; playoffs are already scheduled");
        }

        if (match.Stage == MatchStage.PLAYOFF)
        {
            if (score.GetWinner() == Winner.DRAW)
            {
                return OperationResult.Failure("Playoff matches can't end in a draw");
            }

            if (playoffs.Any(m => m.Round > match.Round))
            {
                return OperationResult.Failure("Next playoff round is already scheduled");
            }
        }

        if (!await matches.UpdateScoreAsync(tournamentId, matchId, score, cancellationToken))
        {
            return OperationResult.NotFound("Match doesn't exist");
        }

        await AdvanceAsync(tournamentId, cancellationToken);
        return OperationResult.Success();
    }

    private async Task AdvanceAsync(string tournamentId, CancellationToken cancellationToken)
    {
        var all = await matches.GetAllAsync(tournamentId, cancellationToken);
        var roundRobin = all.Where(m => m.Stage == MatchStage.ROUND_ROBIN).ToList();
        var playoffs = all.Where(m => m.Stage == MatchStage.PLAYOFF).ToList();

        if (playoffs.Count == 0)
        {
            if (roundRobin.Count == 0 || roundRobin.Any(m => !m.IsPlayed))
            {
                return;
            }

            var tournament = await tournaments.GetAsync(tournamentId, cancellationToken);
            var group = await groups.GetAsync(tournamentId, roundRobin[0].GroupId, cancellationToken);
            if (tournament is null || group is null)
            {
                return;
            }

            var seeds = MatchScheduler.ComputeStandings(group.Teams, roundRobin)
                .Take(tournament.Format.PlayoffTeams)
                .Select(standing => standing.TeamId)
                .ToList();
            await matches.CreateManyAsync(
                MatchScheduler.CreatePlayoffFirstRound(tournament.Id!, group.Id!, seeds),
                cancellationToken);
            return;
        }

        var lastRound = playoffs.Max(m => m.Round);
        var current = playoffs.Where(m => m.Round == lastRound).ToList();
        if (current.Count > 1 && current.All(m => m.IsPlayed))
        {
            await matches.CreateManyAsync(MatchScheduler.CreateNextPlayoffRound(current), cancellationToken);
        }
    }
}
