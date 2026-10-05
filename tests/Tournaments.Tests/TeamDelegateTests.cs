using Tournaments.Core;

namespace Tournaments.Tests;

public sealed class TeamDelegateTests
{
    [Fact]
    public async Task GetTeam_returns_team_from_repository()
    {
        var expected = new Team { Id = Guid.NewGuid().ToString(), Name = "Falcons" };
        var repository = new FakeTeamRepository();
        repository.Teams.Add(expected);
        var teamDelegate = new TeamDelegate(repository);

        var actual = await teamDelegate.GetTeamAsync(expected.Id);

        Assert.Same(expected, actual);
    }

    [Fact]
    public async Task GetAllTeams_returns_teams_from_repository()
    {
        var expected = new Team { Id = Guid.NewGuid().ToString(), Name = "Falcons" };
        var repository = new FakeTeamRepository();
        repository.Teams.Add(expected);
        var teamDelegate = new TeamDelegate(repository);

        var actual = await teamDelegate.GetAllTeamsAsync();

        Assert.Same(repository.Teams, actual);
    }

    [Fact]
    public async Task SaveTeam_returns_created_identifier()
    {
        var team = new Team { Name = "Falcons" };
        var repository = new FakeTeamRepository();
        var teamDelegate = new TeamDelegate(repository);

        var id = await teamDelegate.SaveTeamAsync(team);

        Assert.Equal(team.Id, id);
        Assert.Contains(team, repository.Teams);
    }
}
