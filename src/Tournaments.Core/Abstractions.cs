namespace Tournaments.Core;

public interface ITeamRepository
{
    Task<Team?> GetAsync(string id, CancellationToken cancellationToken = default);
    Task<IReadOnlyList<Team>> GetAllAsync(CancellationToken cancellationToken = default);
    Task<string> CreateAsync(Team team, CancellationToken cancellationToken = default);
}

public interface ITournamentRepository
{
    Task<Tournament?> GetAsync(string id, CancellationToken cancellationToken = default);
    Task<IReadOnlyList<Tournament>> GetAllAsync(CancellationToken cancellationToken = default);
    Task<string> CreateAsync(Tournament tournament, CancellationToken cancellationToken = default);
}

public interface IGroupRepository
{
    Task<Group?> GetAsync(string tournamentId, string groupId, CancellationToken cancellationToken = default);
    Task<Group?> FindByTeamAsync(string tournamentId, string teamId, CancellationToken cancellationToken = default);
    Task<IReadOnlyList<Group>> GetAllAsync(string tournamentId, CancellationToken cancellationToken = default);
    Task<string> CreateAsync(Group group, CancellationToken cancellationToken = default);

    /// <summary>
    /// Atomically appends <paramref name="teams"/> when none is already in the group and the result
    /// stays within <paramref name="maxTeams"/>. Returns false, writing nothing, otherwise.
    /// </summary>
    Task<bool> AddTeamsAsync(
        string groupId,
        IReadOnlyList<Team> teams,
        int maxTeams,
        CancellationToken cancellationToken = default);
}

public interface IMatchRepository
{
    Task<Match?> GetAsync(string tournamentId, string matchId, CancellationToken cancellationToken = default);
    Task<IReadOnlyList<Match>> GetAllAsync(string tournamentId, CancellationToken cancellationToken = default);

    /// <summary>
    /// Inserts the matches in one transaction, skipping any whose (tournament, stage, round, match number)
    /// slot already exists, so repeated generation is harmless. Returns the number inserted.
    /// </summary>
    Task<int> CreateManyAsync(IReadOnlyList<Match> matches, CancellationToken cancellationToken = default);

    Task<bool> UpdateScoreAsync(
        string tournamentId,
        string matchId,
        Score score,
        CancellationToken cancellationToken = default);
}

public interface IMessagePublisher
{
    Task PublishAsync(string queue, string body, CancellationToken cancellationToken = default);
}

public readonly record struct OperationResult(bool IsSuccess, string? Error, bool IsNotFound = false)
{
    public static OperationResult Success() => new(true, null);
    public static OperationResult Failure(string error) => new(false, error);
    public static OperationResult NotFound(string error) => new(false, error, true);
}

public readonly record struct OperationResult<T>(T? Value, string? Error)
{
    public bool IsSuccess => Error is null;
    public static OperationResult<T> Success(T value) => new(value, null);
    public static OperationResult<T> Failure(string error) => new(default, error);
}
