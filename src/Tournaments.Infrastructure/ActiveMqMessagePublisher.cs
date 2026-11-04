using Apache.NMS;
using Apache.NMS.ActiveMQ;
using Tournaments.Core;

namespace Tournaments.Infrastructure;

/// <summary>
/// Publishes over one broker connection shared for the lifetime of the (singleton) publisher.
/// Connections are thread-safe but sessions are not, so each publish opens its own lightweight
/// session and producer on the shared connection. A failed connection is discarded and the next
/// publish reconnects.
/// </summary>
public sealed class ActiveMqMessagePublisher(string brokerUri) : IMessagePublisher, IAsyncDisposable, IDisposable
{
    private readonly ConnectionFactory factory = new(brokerUri);
    private readonly SemaphoreSlim connectionLock = new(1, 1);
    private IConnection? connection;
    private bool disposed;

    public async Task PublishAsync(string queue, string body, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var shared = await GetConnectionAsync(cancellationToken);
        try
        {
            using var session = await shared.CreateSessionAsync(AcknowledgementMode.AutoAcknowledge);
            using var producer = await session.CreateProducerAsync(await session.GetQueueAsync(queue));
            producer.DeliveryMode = MsgDeliveryMode.Persistent;
            await producer.SendAsync(await session.CreateTextMessageAsync(body));
        }
        catch (NMSException)
        {
            await DiscardAsync(shared);
            throw;
        }
    }

    public async ValueTask DisposeAsync()
    {
        await connectionLock.WaitAsync();
        try
        {
            disposed = true;
            if (connection is not null)
            {
                await CloseQuietlyAsync(connection);
                connection = null;
            }
        }
        finally
        {
            connectionLock.Release();
        }
    }

    public void Dispose() => DisposeAsync().AsTask().GetAwaiter().GetResult();

    private async Task<IConnection> GetConnectionAsync(CancellationToken cancellationToken)
    {
        if (Volatile.Read(ref connection) is { } existing)
        {
            return existing;
        }

        await connectionLock.WaitAsync(cancellationToken);
        try
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            if (connection is null)
            {
                var created = await factory.CreateConnectionAsync();
                // Without a failover transport a dropped socket surfaces here rather than on a send.
                created.ExceptionListener += exception => _ = DiscardAsync(created);
                Volatile.Write(ref connection, created);
            }

            return connection;
        }
        finally
        {
            connectionLock.Release();
        }
    }

    private async Task DiscardAsync(IConnection failed)
    {
        // Only clear the field if no other caller has already replaced the failed connection.
        if (Interlocked.CompareExchange(ref connection, null, failed) == failed)
        {
            await CloseQuietlyAsync(failed);
        }
    }

    private static async Task CloseQuietlyAsync(IConnection target)
    {
        try
        {
            await target.CloseAsync();
        }
        catch (NMSException)
        {
            // Already broken; nothing more to release.
        }
        finally
        {
            target.Dispose();
        }
    }
}
