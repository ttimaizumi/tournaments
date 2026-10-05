using System.Text.Json;

namespace Tournaments.Core;

public sealed class TournamentService(ITournamentRepository tournaments, IMessagePublisher messages)
{
    public Task<IReadOnlyList<Tournament>> GetAllAsync(CancellationToken cancellationToken = default) =>
        tournaments.GetAllAsync(cancellationToken);

    public async Task<string> CreateAsync(Tournament tournament, CancellationToken cancellationToken = default)
    {
        var id = await tournaments.CreateAsync(tournament, cancellationToken);
        await messages.PublishAsync("tournament.created", id, cancellationToken);
        return id;
    }
}

public sealed class GroupService(
    ITournamentRepository tournaments,
    IGroupRepository groups,
    ITeamRepository teams,
    IMessagePublisher messages)
{
    public Task<IReadOnlyList<Group>> GetAllAsync(string tournamentId, CancellationToken cancellationToken = default) =>
        groups.GetAllAsync(tournamentId, cancellationToken);

    public Task<Group?> GetAsync(string tournamentId, string groupId, CancellationToken cancellationToken = default) =>
        groups.GetAsync(tournamentId, groupId, cancellationToken);

    public async Task<OperationResult<string>> CreateAsync(
        string tournamentId,
        Group group,
        CancellationToken cancellationToken = default)
    {
        var tournament = await tournaments.GetAsync(tournamentId, cancellationToken);
        if (tournament is null)
        {
            return OperationResult<string>.Failure("Tournament doesn't exist");
        }

        group.TournamentId = tournament.Id!;
        foreach (var requestedTeam in group.Teams)
        {
            if (requestedTeam.Id is null || await teams.GetAsync(requestedTeam.Id, cancellationToken) is null)
            {
                return OperationResult<string>.Failure("Team doesn't exist");
            }
        }

        return OperationResult<string>.Success(await groups.CreateAsync(group, cancellationToken));
    }

    public async Task<OperationResult> AddTeamsAsync(
        string tournamentId,
        string groupId,
        IReadOnlyCollection<Team> requestedTeams,
        CancellationToken cancellationToken = default)
    {
        var group = await groups.GetAsync(tournamentId, groupId, cancellationToken);
        if (group is null)
        {
            return OperationResult.Failure("Group doesn't exist");
        }

        if (group.Teams.Count + requestedTeams.Count > 32)
        {
            return OperationResult.Failure("Group at max capacity");
        }

        var persistedTeams = new List<Team>(requestedTeams.Count);
        var batchIds = new HashSet<string>(StringComparer.Ordinal);
        foreach (var requestedTeam in requestedTeams)
        {
            if (requestedTeam.Id is null || !batchIds.Add(requestedTeam.Id) ||
                await groups.FindByTeamAsync(tournamentId, requestedTeam.Id, cancellationToken) is not null)
            {
                return OperationResult.Failure($"Team {requestedTeam.Id} already exist");
            }

            var persistedTeam = await teams.GetAsync(requestedTeam.Id, cancellationToken);
            if (persistedTeam is null)
            {
                return OperationResult.Failure($"Team {requestedTeam.Id} doesn't exist");
            }

            persistedTeams.Add(persistedTeam);
        }

        foreach (var team in persistedTeams)
        {
            await groups.AddTeamAsync(groupId, team, cancellationToken);
            var body = JsonSerializer.Serialize(
                new TeamAddEvent
                {
                    TournamentId = tournamentId,
                    GroupId = groupId,
                    TeamId = team.Id!
                },
                new JsonSerializerOptions(JsonSerializerDefaults.Web));
            await messages.PublishAsync("tournament.team-add", body, cancellationToken);
        }

        return OperationResult.Success();
    }
}
