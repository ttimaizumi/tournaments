using System.Text.Json;
using Tournaments.Core;

namespace Tournaments.Tests;

public sealed class GroupServiceTests
{
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
        var service = new GroupService(new FakeTournamentRepository(), groupRepository, teamRepository, publisher);

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
        var service = new GroupService(
            new FakeTournamentRepository(), groupRepository, teamRepository, new FakeMessagePublisher());

        var result = await service.AddTeamsAsync(tournamentId, groupId, [team, team]);

        Assert.False(result.IsSuccess);
        Assert.Empty(groupRepository.Groups[0].Teams);
    }
}
