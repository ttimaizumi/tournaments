using System.Text.Json;
using Tournaments.Core;

namespace Tournaments.Tests;

public sealed class GroupDelegateTests
{
    [Fact]
    public async Task GetGroup_returns_group_from_repository()
    {
        var expected = new Group { Id = Guid.NewGuid().ToString(), TournamentId = Guid.NewGuid().ToString() };
        var groupRepository = new FakeGroupRepository();
        groupRepository.Groups.Add(expected);
        var groupDelegate = CreateDelegate(groupRepository: groupRepository);

        var actual = await groupDelegate.GetGroupAsync(expected.TournamentId, expected.Id);

        Assert.Same(expected, actual);
    }

    [Fact]
    public async Task GetAllGroups_returns_only_groups_for_tournament()
    {
        var tournamentId = Guid.NewGuid().ToString();
        var expected = new Group { Id = Guid.NewGuid().ToString(), TournamentId = tournamentId };
        var groupRepository = new FakeGroupRepository();
        groupRepository.Groups.Add(expected);
        groupRepository.Groups.Add(new Group { Id = Guid.NewGuid().ToString(), TournamentId = Guid.NewGuid().ToString() });
        var groupDelegate = CreateDelegate(groupRepository: groupRepository);

        var actual = await groupDelegate.GetAllGroupsAsync(tournamentId);

        Assert.Same(expected, Assert.Single(actual));
    }

    [Fact]
    public async Task SaveGroup_returns_created_identifier()
    {
        var tournament = new Tournament { Id = Guid.NewGuid().ToString() };
        var tournamentRepository = new FakeTournamentRepository();
        tournamentRepository.Tournaments.Add(tournament);
        var groupRepository = new FakeGroupRepository();
        var group = new Group();
        var groupDelegate = CreateDelegate(tournamentRepository, groupRepository);

        var result = await groupDelegate.SaveGroupAsync(tournament.Id, group);

        Assert.True(result.IsSuccess);
        Assert.Equal(group.Id, result.Value);
        Assert.Equal(tournament.Id, group.TournamentId);
        Assert.Contains(group, groupRepository.Groups);
    }

    [Fact]
    public async Task SaveGroup_rejects_unknown_tournament()
    {
        var groupRepository = new FakeGroupRepository();
        var groupDelegate = CreateDelegate(groupRepository: groupRepository);

        var result = await groupDelegate.SaveGroupAsync(Guid.NewGuid().ToString(), new Group());

        Assert.False(result.IsSuccess);
        Assert.Equal("Tournament doesn't exist", result.Error);
        Assert.Empty(groupRepository.Groups);
    }

    [Fact]
    public async Task SaveGroup_rejects_unknown_team()
    {
        var tournament = new Tournament { Id = Guid.NewGuid().ToString() };
        var tournamentRepository = new FakeTournamentRepository();
        tournamentRepository.Tournaments.Add(tournament);
        var groupRepository = new FakeGroupRepository();
        var group = new Group { Teams = [new Team { Id = Guid.NewGuid().ToString() }] };
        var groupDelegate = CreateDelegate(tournamentRepository, groupRepository);

        var result = await groupDelegate.SaveGroupAsync(tournament.Id, group);

        Assert.False(result.IsSuccess);
        Assert.Equal("Team doesn't exist", result.Error);
        Assert.Empty(groupRepository.Groups);
    }

    [Fact]
    public async Task AddTeams_allows_group_to_reach_32_and_publishes_compatible_event()
    {
        var tournamentId = Guid.NewGuid().ToString();
        var groupId = Guid.NewGuid().ToString();
        var newTeam = new Team { Id = Guid.NewGuid().ToString(), Name = "Team 32" };
        var teamRepository = new FakeTeamRepository();
        teamRepository.Teams.Add(newTeam);
        var groupRepository = new FakeGroupRepository();
        groupRepository.Groups.Add(new Group
        {
            Id = groupId,
            TournamentId = tournamentId,
            Teams = Enumerable.Range(1, 31)
                .Select(number => new Team { Id = Guid.NewGuid().ToString(), Name = $"Team {number}" })
                .ToList()
        });
        var publisher = new FakeMessagePublisher();
        var service = new GroupDelegate(new FakeTournamentRepository(), groupRepository, teamRepository, publisher);

        var result = await service.AddTeamsAsync(tournamentId, groupId, [newTeam]);

        Assert.True(result.IsSuccess);
        Assert.Equal(32, groupRepository.Groups[0].Teams.Count);
        var message = Assert.Single(publisher.Messages);
        Assert.Equal("tournament.team-add", message.Queue);
        using var body = JsonDocument.Parse(message.Body);
        Assert.Equal(tournamentId, body.RootElement.GetProperty("tournamentId").GetString());
        Assert.Equal(groupId, body.RootElement.GetProperty("groupId").GetString());
        Assert.Equal(newTeam.Id, body.RootElement.GetProperty("teamId").GetString());
    }

    [Fact]
    public async Task AddTeams_rejects_duplicate_ids_in_one_request_before_writing()
    {
        var tournamentId = Guid.NewGuid().ToString();
        var groupId = Guid.NewGuid().ToString();
        var team = new Team { Id = Guid.NewGuid().ToString(), Name = "Falcons" };
        var teamRepository = new FakeTeamRepository();
        teamRepository.Teams.Add(team);
        var groupRepository = new FakeGroupRepository();
        groupRepository.Groups.Add(new Group { Id = groupId, TournamentId = tournamentId });
        var service = new GroupDelegate(
            new FakeTournamentRepository(), groupRepository, teamRepository, new FakeMessagePublisher());

        var result = await service.AddTeamsAsync(tournamentId, groupId, [team, team]);

        Assert.False(result.IsSuccess);
        Assert.Empty(groupRepository.Groups[0].Teams);
    }

    private static GroupDelegate CreateDelegate(
        FakeTournamentRepository? tournamentRepository = null,
        FakeGroupRepository? groupRepository = null) =>
        new(
            tournamentRepository ?? new FakeTournamentRepository(),
            groupRepository ?? new FakeGroupRepository(),
            new FakeTeamRepository(),
            new FakeMessagePublisher());
}
