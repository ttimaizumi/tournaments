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
    Task AddTeamAsync(string groupId, Team team, CancellationToken cancellationToken = default);
}

public interface IMessagePublisher
{
    Task PublishAsync(string queue, string body, CancellationToken cancellationToken = default);
}

public readonly record struct OperationResult(bool IsSuccess, string? Error)
{
    public static OperationResult Success() => new(true, null);
    public static OperationResult Failure(string error) => new(false, error);
}

public readonly record struct OperationResult<T>(T? Value, string? Error)
{
    public bool IsSuccess => Error is null;
    public static OperationResult<T> Success(T value) => new(value, null);
    public static OperationResult<T> Failure(string error) => new(default, error);
}
