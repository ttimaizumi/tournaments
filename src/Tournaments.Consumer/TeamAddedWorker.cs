using System.Text.Json;
using Apache.NMS;
using Apache.NMS.ActiveMQ;
using Tournaments.Core;

namespace Tournaments.Consumer;

public sealed class TeamAddedWorker(
    IConfiguration configuration,
    IGroupRepository groups,
    ILogger<TeamAddedWorker> logger) : BackgroundService
{
    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var brokerUri = configuration["ActiveMq:BrokerUri"]
            ?? throw new InvalidOperationException("ActiveMq:BrokerUri is required.");
        var factory = new ConnectionFactory(brokerUri);
        using var connection = factory.CreateConnection();
        connection.Start();
        using var session = connection.CreateSession(AcknowledgementMode.AutoAcknowledge);
        using var consumer = session.CreateConsumer(session.GetQueue("tournament.team-add"));

        logger.LogInformation("Listening on tournament.team-add");
        while (!stoppingToken.IsCancellationRequested)
        {
            var message = consumer.Receive(TimeSpan.FromMilliseconds(1500));
            if (message is not ITextMessage textMessage)
            {
                continue;
            }

            try
            {
                var teamAdded = JsonSerializer.Deserialize<TeamAddEvent>(textMessage.Text, new JsonSerializerOptions(JsonSerializerDefaults.Web));
                if (teamAdded is null)
                {
                    continue;
                }

                var group = await groups.GetAsync(teamAdded.TournamentId, teamAdded.GroupId, stoppingToken);
                if (group?.Teams.Count == 32)
                {
                    logger.LogInformation("Group {GroupId} is full; match generation is not implemented", teamAdded.GroupId);
                }
            }
            catch (JsonException exception)
            {
                logger.LogWarning(exception, "Ignoring malformed tournament.team-add message");
            }
        }
    }
}
