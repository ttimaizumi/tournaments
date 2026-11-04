using Tournaments.Core;

namespace Tournaments.Tests;

internal sealed class FakeTeamRepository : ITeamRepository
{
    public List<Team> Teams { get; } = [];

    public Task<Team?> GetAsync(string id, CancellationToken cancellationToken = default) =>
        Task.FromResult(Teams.SingleOrDefault(team => team.Id == id));

    public Task<IReadOnlyList<Team>> GetAllAsync(CancellationToken cancellationToken = default) =>
        Task.FromResult<IReadOnlyList<Team>>(Teams);

    public Task<string> CreateAsync(Team team, CancellationToken cancellationToken = default)
    {
        team.Id ??= Guid.NewGuid().ToString();
        Teams.Add(team);
        return Task.FromResult(team.Id);
    }
}

internal sealed class FakeTournamentRepository : ITournamentRepository
{
    public List<Tournament> Tournaments { get; } = [];

    public Task<Tournament?> GetAsync(string id, CancellationToken cancellationToken = default) =>
        Task.FromResult(Tournaments.SingleOrDefault(tournament => tournament.Id == id));

    public Task<IReadOnlyList<Tournament>> GetAllAsync(CancellationToken cancellationToken = default) =>
        Task.FromResult<IReadOnlyList<Tournament>>(Tournaments);

    public Task<string> CreateAsync(Tournament tournament, CancellationToken cancellationToken = default)
    {
        tournament.Id ??= Guid.NewGuid().ToString();
        Tournaments.Add(tournament);
        return Task.FromResult(tournament.Id);
    }
}

internal sealed class FakeGroupRepository : IGroupRepository
{
    public List<Group> Groups { get; } = [];

    public Task<Group?> GetAsync(string tournamentId, string groupId, CancellationToken cancellationToken = default) =>
        Task.FromResult(Groups.SingleOrDefault(group => group.TournamentId == tournamentId && group.Id == groupId));

    public Task<Group?> FindByTeamAsync(string tournamentId, string teamId, CancellationToken cancellationToken = default) =>
        Task.FromResult(Groups.SingleOrDefault(group =>
            group.TournamentId == tournamentId && group.Teams.Any(team => team.Id == teamId)));

    public Task<IReadOnlyList<Group>> GetAllAsync(string tournamentId, CancellationToken cancellationToken = default) =>
        Task.FromResult<IReadOnlyList<Group>>(Groups.Where(group => group.TournamentId == tournamentId).ToList());

    public Task<string> CreateAsync(Group group, CancellationToken cancellationToken = default)
    {
        group.Id ??= Guid.NewGuid().ToString();
        Groups.Add(group);
        return Task.FromResult(group.Id);
    }

    public Task<bool> AddTeamsAsync(
        string groupId,
        IReadOnlyList<Team> teams,
        int maxTeams,
        CancellationToken cancellationToken = default)
    {
        var group = Groups.Single(group => group.Id == groupId);
        if (group.Teams.Count + teams.Count > maxTeams || group.Teams.Any(existing => teams.Any(t => t.Id == existing.Id)))
        {
            return Task.FromResult(false);
        }

        group.Teams.AddRange(teams);
        return Task.FromResult(true);
    }
}

internal sealed class FakeMatchRepository : IMatchRepository
{
    public List<Match> Matches { get; } = [];

    public Task<Match?> GetAsync(string tournamentId, string matchId, CancellationToken cancellationToken = default) =>
        Task.FromResult(Matches.SingleOrDefault(match => match.TournamentId == tournamentId && match.Id == matchId));

    public Task<IReadOnlyList<Match>> GetAllAsync(string tournamentId, CancellationToken cancellationToken = default) =>
        Task.FromResult<IReadOnlyList<Match>>(Matches.Where(match => match.TournamentId == tournamentId).ToList());

    public Task<int> CreateManyAsync(IReadOnlyList<Match> matches, CancellationToken cancellationToken = default)
    {
        var inserted = 0;
        foreach (var match in matches)
        {
            if (Matches.Any(existing => existing.TournamentId == match.TournamentId && existing.Stage == match.Stage &&
                                        existing.Round == match.Round && existing.MatchNumber == match.MatchNumber))
            {
                continue;
            }

            match.Id ??= Guid.NewGuid().ToString();
            Matches.Add(match);
            inserted++;
        }

        return Task.FromResult(inserted);
    }

    public Task<bool> UpdateScoreAsync(
        string tournamentId,
        string matchId,
        Score score,
        CancellationToken cancellationToken = default)
    {
        var match = Matches.SingleOrDefault(match => match.TournamentId == tournamentId && match.Id == matchId);
        if (match is null)
        {
            return Task.FromResult(false);
        }

        match.Score = score;
        return Task.FromResult(true);
    }
}

internal sealed class FakeMessagePublisher : IMessagePublisher
{
    public List<(string Queue, string Body)> Messages { get; } = [];

    public Task PublishAsync(string queue, string body, CancellationToken cancellationToken = default)
    {
        Messages.Add((queue, body));
        return Task.CompletedTask;
    }
}
