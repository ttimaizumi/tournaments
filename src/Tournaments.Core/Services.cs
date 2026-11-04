namespace Tournaments.Core;

public sealed class TournamentService(ITournamentRepository tournaments, IMessagePublisher messages)
{
    public Task<IReadOnlyList<Tournament>> GetAllAsync(CancellationToken cancellationToken = default) =>
        tournaments.GetAllAsync(cancellationToken);

    public async Task<OperationResult<string>> CreateAsync(Tournament tournament, CancellationToken cancellationToken = default)
    {
        if (tournament.Format.Validate() is { } error)
        {
            return OperationResult<string>.Failure(error);
        }

        var id = await tournaments.CreateAsync(tournament, cancellationToken);
        await messages.PublishAsync("tournament.created", id, cancellationToken);
        return OperationResult<string>.Success(id);
    }
}
