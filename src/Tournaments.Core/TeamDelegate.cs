namespace Tournaments.Core;

public sealed class TeamDelegate(ITeamRepository repository)
{
    public Task<Team?> GetTeamAsync(string id, CancellationToken cancellationToken = default) =>
        repository.GetAsync(id, cancellationToken);

    public Task<IReadOnlyList<Team>> GetAllTeamsAsync(CancellationToken cancellationToken = default) =>
        repository.GetAllAsync(cancellationToken);

    public Task<string> SaveTeamAsync(Team team, CancellationToken cancellationToken = default) =>
        repository.CreateAsync(team, cancellationToken);
}
