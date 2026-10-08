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
