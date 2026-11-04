using Tournaments.Core;

namespace Tournaments.Tests;

public sealed class MatchSchedulerTests
{
    [Fact]
    public void RoundRobin_for_32_teams_pairs_every_team_once()
    {
        var teams = GroupDelegateTests.Teams(32);

        var matches = MatchScheduler.CreateRoundRobin("t", "g", teams);

        Assert.Equal(496, matches.Count);
        Assert.Equal(31, matches.Select(match => match.Round).Distinct().Count());
        Assert.All(matches.GroupBy(match => match.Round), round =>
        {
            Assert.Equal(16, round.Count());
            var playing = round.SelectMany(match => new[] { match.HomeTeamId, match.VisitorTeamId }).ToList();
            Assert.Equal(32, playing.Distinct().Count());
        });
        var pairs = matches
            .Select(match => string.Join('|', new[] { match.HomeTeamId, match.VisitorTeamId }.Order(StringComparer.Ordinal)))
            .ToList();
        Assert.Equal(496, pairs.Distinct().Count());
        Assert.All(matches, match => Assert.NotEqual(match.HomeTeamId, match.VisitorTeamId));
    }

    [Fact]
    public void Playoff_first_round_uses_standard_bracket_order()
    {
        var seeds = Enumerable.Range(1, 8).Select(seed => $"seed{seed}").ToList();

        var matches = MatchScheduler.CreatePlayoffFirstRound("t", "g", seeds);

        Assert.Equal(
            ["seed1-seed8", "seed4-seed5", "seed2-seed7", "seed3-seed6"],
            matches.Select(match => $"{match.HomeTeamId}-{match.VisitorTeamId}"));
    }

    [Fact]
    public void Standings_rank_by_points_then_score_difference()
    {
        // a: W 1-0 + D 1-1 = 4 pts; b: L + W 5-0 = 3 pts (+4); c: L 0-5 + D = 1 pt.
        var teams = GroupDelegateTests.Teams(3);
        var (a, b, c) = (teams[0].Id!, teams[1].Id!, teams[2].Id!);
        Match Played(string home, string visitor, int h, int v) => new()
        {
            Stage = MatchStage.ROUND_ROBIN, HomeTeamId = home, VisitorTeamId = visitor,
            Score = new Score { HomeTeamScore = h, VisitorTeamScore = v }
        };

        var standings = MatchScheduler.ComputeStandings(teams, [Played(a, b, 1, 0), Played(b, c, 5, 0), Played(c, a, 1, 1)]);
        var tied = MatchScheduler.ComputeStandings(teams, [Played(a, c, 1, 0), Played(b, c, 5, 0)]);

        Assert.Equal([a, b, c], standings.Select(standing => standing.TeamId));
        Assert.Equal((4, 1, 1), (standings[0].Points, standings[0].Wins, standings[0].Draws));
        // a and b both have 3 points; b's +5 beats a's +1.
        Assert.Equal([b, a, c], tied.Select(standing => standing.TeamId));
    }
}

public sealed class MatchDelegateTests
{
    private readonly string tournamentId = Guid.NewGuid().ToString();
    private readonly string groupId = Guid.NewGuid().ToString();
    private readonly FakeGroupRepository groups = new();
    private readonly FakeMatchRepository matches = new();
    private readonly MatchDelegate service;

    public MatchDelegateTests()
    {
        service = new MatchDelegate(GroupDelegateTests.TournamentRepositoryWith(tournamentId), groups, matches);
    }

    [Fact]
    public async Task Generation_waits_until_group_is_full()
    {
        AddGroup(31);

        var result = await service.GenerateRoundRobinAsync(tournamentId, groupId);

        Assert.True(result.IsSuccess);
        Assert.Equal(0, result.Value);
        Assert.Empty(matches.Matches);
    }

    [Fact]
    public async Task Generation_creates_schedule_once_for_full_group()
    {
        AddGroup(32);

        var first = await service.GenerateRoundRobinAsync(tournamentId, groupId);
        var second = await service.GenerateRoundRobinAsync(tournamentId, groupId);

        Assert.Equal(496, first.Value);
        Assert.Equal(0, second.Value);
        Assert.Equal(496, matches.Matches.Count);
    }

    [Fact]
    public async Task Generation_rejects_group_with_duplicate_teams()
    {
        var group = AddGroup(31);
        group.Teams.Add(new Team { Id = group.Teams[0].Id, Name = "Duplicate" });

        var result = await service.GenerateRoundRobinAsync(tournamentId, groupId);

        Assert.False(result.IsSuccess);
        Assert.Empty(matches.Matches);
    }

    [Fact]
    public async Task Completing_round_robin_seeds_top_eight_into_playoffs_and_runs_to_a_final()
    {
        var group = AddGroup(32);
        await service.GenerateRoundRobinAsync(tournamentId, groupId);
        // Lower team number always wins, so Team 01 finishes first and Team 08 eighth.
        var rank = group.Teams.Select((team, index) => (team.Id!, index)).ToDictionary();
        foreach (var match in matches.Matches.ToList())
        {
            var homeWins = rank[match.HomeTeamId] < rank[match.VisitorTeamId];
            var result = await service.UpdateScoreAsync(tournamentId, match.Id!, Win(homeWins));
            Assert.True(result.IsSuccess, result.Error);
        }

        var quarterFinals = Playoffs(1);
        Assert.Equal(4, quarterFinals.Count);
        Assert.Equal((group.Teams[0].Id, group.Teams[7].Id), (quarterFinals[0].HomeTeamId, quarterFinals[0].VisitorTeamId));
        Assert.Equal((1, 8), (quarterFinals[0].HomeSeed, quarterFinals[0].VisitorSeed));

        await PlayRoundWithBetterSeedWinning(1);
        var semiFinals = Playoffs(2);
        Assert.Equal(2, semiFinals.Count);
        Assert.Equal((1, 4), (semiFinals[0].HomeSeed, semiFinals[0].VisitorSeed));
        Assert.Equal((2, 3), (semiFinals[1].HomeSeed, semiFinals[1].VisitorSeed));

        await PlayRoundWithBetterSeedWinning(2);
        var final = Assert.Single(Playoffs(3));
        Assert.Equal((1, 2), (final.HomeSeed, final.VisitorSeed));

        await PlayRoundWithBetterSeedWinning(3);
        Assert.Empty(Playoffs(4));
        Assert.Equal(group.Teams[0].Id, Playoffs(3)[0].GetWinnerTeamId());
    }

    [Fact]
    public async Task Round_robin_scores_are_locked_once_playoffs_exist()
    {
        AddGroup(32);
        await service.GenerateRoundRobinAsync(tournamentId, groupId);
        foreach (var match in matches.Matches.ToList())
        {
            await service.UpdateScoreAsync(tournamentId, match.Id!, Win(true));
        }

        var result = await service.UpdateScoreAsync(tournamentId, matches.Matches[0].Id!, Win(false));

        Assert.False(result.IsSuccess);
        Assert.Equal("Round robin is closed; playoffs are already scheduled", result.Error);
    }

    [Fact]
    public async Task Playoff_draw_is_rejected_but_round_robin_draw_is_allowed()
    {
        AddGroup(32);
        await service.GenerateRoundRobinAsync(tournamentId, groupId);
        var draw = new Score { HomeTeamScore = 2, VisitorTeamScore = 2 };

        Assert.True((await service.UpdateScoreAsync(tournamentId, matches.Matches[0].Id!, draw)).IsSuccess);
        foreach (var match in matches.Matches.Skip(1).ToList())
        {
            await service.UpdateScoreAsync(tournamentId, match.Id!, Win(true));
        }

        var result = await service.UpdateScoreAsync(tournamentId, Playoffs(1)[0].Id!, draw);
        Assert.False(result.IsSuccess);
        Assert.Equal("Playoff matches can't end in a draw", result.Error);
    }

    [Fact]
    public async Task UpdateScore_reports_unknown_match_and_negative_scores()
    {
        var missing = await service.UpdateScoreAsync(tournamentId, Guid.NewGuid().ToString(), Win(true));
        var negative = await service.UpdateScoreAsync(
            tournamentId, Guid.NewGuid().ToString(), new Score { HomeTeamScore = -1 });

        Assert.True(missing.IsNotFound);
        Assert.False(negative.IsSuccess);
        Assert.False(negative.IsNotFound);
    }

    private Group AddGroup(int teamCount)
    {
        var group = new Group { Id = groupId, TournamentId = tournamentId, Teams = GroupDelegateTests.Teams(teamCount) };
        groups.Groups.Add(group);
        return group;
    }

    private List<Match> Playoffs(int round) =>
        matches.Matches.Where(m => m.Stage == MatchStage.PLAYOFF && m.Round == round).OrderBy(m => m.MatchNumber).ToList();

    private async Task PlayRoundWithBetterSeedWinning(int round)
    {
        foreach (var match in Playoffs(round))
        {
            var result = await service.UpdateScoreAsync(tournamentId, match.Id!, Win(match.HomeSeed < match.VisitorSeed));
            Assert.True(result.IsSuccess, result.Error);
        }
    }

    private static Score Win(bool home) =>
        home ? new Score { HomeTeamScore = 3, VisitorTeamScore = 1 } : new Score { HomeTeamScore = 1, VisitorTeamScore = 3 };
}
