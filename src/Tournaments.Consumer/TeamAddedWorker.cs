using System.Text.Json;
using Apache.NMS;
using Apache.NMS.ActiveMQ;
using Tournaments.Core;

namespace Tournaments.Consumer;

public sealed class TeamAddedWorker(
    IConfiguration configuration,
    MatchDelegate matches,
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
        using var consumer = session.CreateConsumer(session.GetQueue(GroupDelegate.TeamAddQueue));

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

                // Every team-add message re-checks the group; generation is a no-op until it is full
                // and idempotent afterwards, so redelivered or concurrent messages are harmless.
                var result = await matches.GenerateRoundRobinAsync(teamAdded.TournamentId, teamAdded.GroupId, stoppingToken);
                if (!result.IsSuccess)
                {
                    logger.LogWarning("Skipping match generation for group {GroupId}: {Error}", teamAdded.GroupId, result.Error);
                }
                else if (result.Value > 0)
                {
                    logger.LogInformation(
                        "Group {GroupId} is full; created {MatchCount} round-robin matches", teamAdded.GroupId, result.Value);
                }
            }
            catch (JsonException exception)
            {
                logger.LogWarning(exception, "Ignoring malformed tournament.team-add message");
            }
            catch (Exception exception) when (exception is not OperationCanceledException)
            {
                // Keep consuming; a later team-add message for the same group retries generation.
                logger.LogError(exception, "Match generation failed for tournament.team-add message");
            }
        }
    }
}
