using System.Text.Json;

namespace Tournaments.Core;

public sealed class GroupDelegate(
    ITournamentRepository tournaments,
    IGroupRepository groups,
    ITeamRepository teams,
    IMessagePublisher messages)
{
    public const string TeamAddQueue = "tournament.team-add";

    private static readonly JsonSerializerOptions EventJson = new(JsonSerializerDefaults.Web);

    public Task<IReadOnlyList<Group>> GetAllGroupsAsync(string tournamentId, CancellationToken cancellationToken = default) =>
        groups.GetAllAsync(tournamentId, cancellationToken);

    public Task<Group?> GetGroupAsync(string tournamentId, string groupId, CancellationToken cancellationToken = default) =>
        groups.GetAsync(tournamentId, groupId, cancellationToken);

    public async Task<OperationResult<string>> SaveGroupAsync(
        string tournamentId,
        Group group,
        CancellationToken cancellationToken = default)
    {
        var tournament = await tournaments.GetAsync(tournamentId, cancellationToken);
        if (tournament is null)
        {
            return OperationResult<string>.Failure("Tournament doesn't exist");
        }

        var existingGroups = await groups.GetAllAsync(tournamentId, cancellationToken);
        if (existingGroups.Count >= tournament.Format.NumberOfGroups)
        {
            return OperationResult<string>.Failure(
                $"Tournament already has {tournament.Format.NumberOfGroups} group(s)");
        }

        if (group.Teams.Count > tournament.Format.MaxTeamsPerGroup)
        {
            return OperationResult<string>.Failure("Group at max capacity");
        }

        var batchIds = new HashSet<string>(StringComparer.Ordinal);
        var persistedTeams = new List<Team>(group.Teams.Count);
        foreach (var requestedTeam in group.Teams)
        {
            var persistedTeam = requestedTeam.Id is null ? null : await teams.GetAsync(requestedTeam.Id, cancellationToken);
            if (persistedTeam is null)
            {
                return OperationResult<string>.Failure("Team doesn't exist");
            }

            if (!batchIds.Add(persistedTeam.Id!))
            {
                return OperationResult<string>.Failure($"Team {persistedTeam.Id} already exist");
            }

            persistedTeams.Add(persistedTeam);
        }

        group.TournamentId = tournament.Id!;
        group.Teams = persistedTeams;
        var groupId = await groups.CreateAsync(group, cancellationToken);
        // A group created already full must trigger match generation just like one filled by PATCH.
        await PublishTeamsAddedAsync(tournamentId, groupId, persistedTeams, cancellationToken);
        return OperationResult<string>.Success(groupId);
    }

    public async Task<OperationResult> AddTeamsAsync(
        string tournamentId,
        string groupId,
        IReadOnlyCollection<Team> requestedTeams,
        CancellationToken cancellationToken = default)
    {
        var tournament = await tournaments.GetAsync(tournamentId, cancellationToken);
        if (tournament is null)
        {
            return OperationResult.Failure("Tournament doesn't exist");
        }

        var group = await groups.GetAsync(tournamentId, groupId, cancellationToken);
        if (group is null)
        {
            return OperationResult.Failure("Group doesn't exist");
        }

        var maxTeams = tournament.Format.MaxTeamsPerGroup;
        if (group.Teams.Count + requestedTeams.Count > maxTeams)
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

        // The repository re-checks capacity and uniqueness atomically, closing the race between
        // concurrent requests that each passed the checks above.
        if (!await groups.AddTeamsAsync(groupId, persistedTeams, maxTeams, cancellationToken))
        {
            return OperationResult.Failure("Group at max capacity or team already exist");
        }

        await PublishTeamsAddedAsync(tournamentId, groupId, persistedTeams, cancellationToken);
        return OperationResult.Success();
    }

    private async Task PublishTeamsAddedAsync(
        string tournamentId,
        string groupId,
        IEnumerable<Team> added,
        CancellationToken cancellationToken)
    {
        foreach (var team in added)
        {
            var body = JsonSerializer.Serialize(
                new TeamAddEvent { TournamentId = tournamentId, GroupId = groupId, TeamId = team.Id! },
                EventJson);
            await messages.PublishAsync(TeamAddQueue, body, cancellationToken);
        }
    }
}
