namespace Tournaments.Core;

/// <summary>Pure fixture and bracket generation; no I/O.</summary>
public static class MatchScheduler
{
    /// <summary>
    /// Single round robin using the circle method: every team meets every other team once.
    /// For an even team count n this yields n - 1 rounds of n / 2 matches.
    /// </summary>
    public static IReadOnlyList<Match> CreateRoundRobin(string tournamentId, string groupId, IReadOnlyList<Team> teams)
    {
        var rotation = teams.Select(team => team.Id).ToList();
        if (rotation.Count % 2 == 1)
        {
            rotation.Add(null); // bye
        }

        var count = rotation.Count;
        var fixtures = new List<Match>(count / 2 * (count - 1));
        for (var round = 0; round < count - 1; round++)
        {
            var matchNumber = 1;
            for (var i = 0; i < count / 2; i++)
            {
                var home = rotation[i];
                var visitor = rotation[count - 1 - i];
                // Alternate home advantage so no team hosts every match.
                if (i == 0 ? round % 2 == 1 : i % 2 == 1)
                {
                    (home, visitor) = (visitor, home);
                }

                if (home is null || visitor is null)
                {
                    continue;
                }

                fixtures.Add(new Match
                {
                    TournamentId = tournamentId,
                    GroupId = groupId,
                    Stage = MatchStage.ROUND_ROBIN,
                    Round = round + 1,
                    MatchNumber = matchNumber++,
                    HomeTeamId = home,
                    VisitorTeamId = visitor
                });
            }

            // Keep the first team fixed and rotate everyone else one position clockwise.
            var last = rotation[count - 1];
            rotation.RemoveAt(count - 1);
            rotation.Insert(1, last);
        }

        return fixtures;
    }

    /// <summary>
    /// Ranks teams on round-robin results: points (win 3, draw 1), then score difference,
    /// score for, wins, and finally name and id so the order is deterministic.
    /// </summary>
    public static IReadOnlyList<Standing> ComputeStandings(IReadOnlyList<Team> teams, IEnumerable<Match> roundRobinMatches)
    {
        var table = teams
            .Where(team => team.Id is not null)
            .ToDictionary(team => team.Id!, team => new Standing { TeamId = team.Id!, TeamName = team.Name });

        foreach (var match in roundRobinMatches.Where(match => match.Stage == MatchStage.ROUND_ROBIN && match.IsPlayed))
        {
            if (!table.TryGetValue(match.HomeTeamId, out var home) ||
                !table.TryGetValue(match.VisitorTeamId, out var visitor))
            {
                continue;
            }

            var score = match.Score!;
            Record(home, score.HomeTeamScore, score.VisitorTeamScore);
            Record(visitor, score.VisitorTeamScore, score.HomeTeamScore);
        }

        var ranked = table.Values
            .OrderByDescending(standing => standing.Points)
            .ThenByDescending(standing => standing.ScoreDifference)
            .ThenByDescending(standing => standing.ScoreFor)
            .ThenByDescending(standing => standing.Wins)
            .ThenBy(standing => standing.TeamName, StringComparer.Ordinal)
            .ThenBy(standing => standing.TeamId, StringComparer.Ordinal)
            .ToList();
        for (var i = 0; i < ranked.Count; i++)
        {
            ranked[i].Rank = i + 1;
        }

        return ranked;
    }

    /// <summary>
    /// First playoff round for the given seeds (seed 1 first). Uses standard bracket order
    /// (1v8, 4v5, 2v7, 3v6 for eight teams) so the top two seeds can only meet in the final.
    /// </summary>
    public static IReadOnlyList<Match> CreatePlayoffFirstRound(
        string tournamentId,
        string groupId,
        IReadOnlyList<string> seededTeamIds)
    {
        var order = BracketOrder(seededTeamIds.Count);
        var fixtures = new List<Match>(order.Count / 2);
        for (var i = 0; i < order.Count; i += 2)
        {
            fixtures.Add(new Match
            {
                TournamentId = tournamentId,
                GroupId = groupId,
                Stage = MatchStage.PLAYOFF,
                Round = 1,
                MatchNumber = i / 2 + 1,
                HomeSeed = order[i],
                VisitorSeed = order[i + 1],
                HomeTeamId = seededTeamIds[order[i] - 1],
                VisitorTeamId = seededTeamIds[order[i + 1] - 1]
            });
        }

        return fixtures;
    }

    /// <summary>Pairs winners of adjacent bracket slots; the better seed hosts.</summary>
    public static IReadOnlyList<Match> CreateNextPlayoffRound(IReadOnlyList<Match> completedRound)
    {
        var ordered = completedRound.OrderBy(match => match.MatchNumber).ToList();
        var fixtures = new List<Match>(ordered.Count / 2);
        for (var i = 0; i + 1 < ordered.Count; i += 2)
        {
            var first = Advance(ordered[i]);
            var second = Advance(ordered[i + 1]);
            if (second.Seed < first.Seed)
            {
                (first, second) = (second, first);
            }

            fixtures.Add(new Match
            {
                TournamentId = ordered[i].TournamentId,
                GroupId = ordered[i].GroupId,
                Stage = MatchStage.PLAYOFF,
                Round = ordered[i].Round + 1,
                MatchNumber = i / 2 + 1,
                HomeTeamId = first.TeamId,
                HomeSeed = first.Seed,
                VisitorTeamId = second.TeamId,
                VisitorSeed = second.Seed
            });
        }

        return fixtures;
    }

    internal static IReadOnlyList<int> BracketOrder(int size)
    {
        var order = new List<int> { 1 };
        while (order.Count < size)
        {
            var sum = order.Count * 2 + 1;
            order = order.SelectMany(seed => new[] { seed, sum - seed }).ToList();
        }

        return order;
    }

    private static (string TeamId, int Seed) Advance(Match match) => match.Score?.GetWinner() switch
    {
        Winner.HOME => (match.HomeTeamId, match.HomeSeed ?? int.MaxValue),
        Winner.VISITOR => (match.VisitorTeamId, match.VisitorSeed ?? int.MaxValue),
        _ => throw new InvalidOperationException($"Playoff match {match.Id} has no winner.")
    };

    private static void Record(Standing standing, int scored, int conceded)
    {
        standing.Played++;
        standing.ScoreFor += scored;
        standing.ScoreAgainst += conceded;
        if (scored > conceded)
        {
            standing.Wins++;
        }
        else if (scored < conceded)
        {
            standing.Losses++;
        }
        else
        {
            standing.Draws++;
        }
    }
}
