using Apache.NMS;
using Apache.NMS.ActiveMQ;
using Tournaments.Core;

namespace Tournaments.Infrastructure;

public sealed class ActiveMqMessagePublisher(string brokerUri) : IMessagePublisher
{
    public Task PublishAsync(string queue, string body, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var factory = new ConnectionFactory(brokerUri);
        using var connection = factory.CreateConnection();
        using var session = connection.CreateSession(AcknowledgementMode.AutoAcknowledge);
        using var producer = session.CreateProducer(session.GetQueue(queue));
        producer.DeliveryMode = MsgDeliveryMode.Persistent;
        producer.Send(session.CreateTextMessage(body));
        return Task.CompletedTask;
    }
}
