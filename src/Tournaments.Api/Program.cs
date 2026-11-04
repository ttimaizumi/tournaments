using Npgsql;
using OpenTelemetry.Metrics;
using OpenTelemetry.Resources;
using OpenTelemetry.Trace;
using Tournaments.Core;
using Tournaments.Infrastructure;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddOpenTelemetry()
    .ConfigureResource(resource => resource.AddService(
        builder.Configuration["OTEL_SERVICE_NAME"] ?? "tournaments-api"))
    .WithMetrics(metrics => metrics
        .AddAspNetCoreInstrumentation()
        .AddRuntimeInstrumentation()
        .AddMeter("Npgsql")
        .AddView("http.server.request.duration", new ExplicitBucketHistogramConfiguration
        {
            Boundaries = [0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10]
        })
        .AddPrometheusExporter())
    .WithTracing(tracing =>
    {
        tracing.AddAspNetCoreInstrumentation(options =>
            options.Filter = context =>
                !context.Request.Path.StartsWithSegments("/metrics") &&
                !context.Request.Path.StartsWithSegments("/health"))
            .AddNpgsql();

        // Do not send traces to an implicit localhost collector when none is configured.
        if (!string.IsNullOrWhiteSpace(builder.Configuration["OTEL_EXPORTER_OTLP_ENDPOINT"]) ||
            !string.IsNullOrWhiteSpace(builder.Configuration["OTEL_EXPORTER_OTLP_TRACES_ENDPOINT"]))
        {
            tracing.AddOtlpExporter();
        }
    });

builder.Services.ConfigureHttpJsonOptions(options =>
    options.SerializerOptions.DefaultIgnoreCondition = System.Text.Json.Serialization.JsonIgnoreCondition.WhenWritingNull);

builder.Services.AddSingleton(_ => new NpgsqlDataSourceBuilder(
    builder.Configuration.GetConnectionString("Tournaments")
    ?? throw new InvalidOperationException("ConnectionStrings:Tournaments is required."))
{
    // Keep connection details out of pool-name metric labels.
    Name = "tournament-db"
}.Build());
builder.Services.AddScoped<ITeamRepository, TeamRepository>();
builder.Services.AddScoped<ITournamentRepository, TournamentRepository>();
builder.Services.AddScoped<IGroupRepository, GroupRepository>();
builder.Services.AddScoped<IMatchRepository, MatchRepository>();
builder.Services.AddSingleton<IMessagePublisher>(_ => new ActiveMqMessagePublisher(
    builder.Configuration["ActiveMq:BrokerUri"]
    ?? throw new InvalidOperationException("ActiveMq:BrokerUri is required.")));
builder.Services.AddScoped<TeamDelegate>();
builder.Services.AddScoped<GroupDelegate>();
builder.Services.AddScoped<TournamentService>();
builder.Services.AddScoped<MatchDelegate>();

var app = builder.Build();

app.MapPrometheusScrapingEndpoint();

app.MapGet("/health", () => Results.Text("Services running"));

app.MapGet("/teams", async (TeamDelegate teams, CancellationToken cancellationToken) =>
    Results.Json(await teams.GetAllTeamsAsync(cancellationToken)));

app.MapGet("/teams/{teamId}", async (string teamId, TeamDelegate teams, CancellationToken cancellationToken) =>
{
    if (!System.Text.RegularExpressions.Regex.IsMatch(teamId, "^[A-Za-z0-9-]+$"))
    {
        return Results.Text("Invalid ID format", statusCode: StatusCodes.Status400BadRequest);
    }

    var team = await teams.GetTeamAsync(teamId, cancellationToken);
    return team is null
        ? Results.Text("team not found", statusCode: StatusCodes.Status404NotFound)
        : Results.Json(team);
});

app.MapPost("/teams", async (Team team, HttpContext context, TeamDelegate teams, CancellationToken cancellationToken) =>
{
    var id = await teams.SaveTeamAsync(team, cancellationToken);
    context.Response.Headers.Location = id;
    return Results.StatusCode(StatusCodes.Status201Created);
});

app.MapGet("/tournaments", async (TournamentService tournaments, CancellationToken cancellationToken) =>
    Results.Json(await tournaments.GetAllAsync(cancellationToken)));

app.MapPost("/tournaments", async (
    Tournament tournament,
    HttpContext context,
    TournamentService tournaments,
    CancellationToken cancellationToken) =>
{
    var result = await tournaments.CreateAsync(tournament, cancellationToken);
    if (!result.IsSuccess)
    {
        return Results.Text(result.Error, statusCode: StatusCodes.Status422UnprocessableEntity);
    }

    context.Response.Headers.Location = result.Value;
    return Results.StatusCode(StatusCodes.Status201Created);
});

app.MapGet("/tournaments/{tournamentId}/groups", async (
    string tournamentId,
    GroupDelegate groups,
    CancellationToken cancellationToken) => Results.Json(await groups.GetAllGroupsAsync(tournamentId, cancellationToken)));

app.MapGet("/tournaments/{tournamentId}/groups/{groupId}", async (
    string tournamentId,
    string groupId,
    GroupDelegate groups,
    CancellationToken cancellationToken) =>
{
    var group = await groups.GetGroupAsync(tournamentId, groupId, cancellationToken);
    return group is null ? Results.NotFound() : Results.Json(group);
});

app.MapPost("/tournaments/{tournamentId}/groups", async (
    string tournamentId,
    Group group,
    HttpContext context,
    GroupDelegate groups,
    CancellationToken cancellationToken) =>
{
    var result = await groups.SaveGroupAsync(tournamentId, group, cancellationToken);
    if (!result.IsSuccess)
    {
        return Results.Text(result.Error, statusCode: StatusCodes.Status422UnprocessableEntity);
    }

    context.Response.Headers.Location = result.Value;
    return Results.StatusCode(StatusCodes.Status201Created);
});

app.MapMethods("/tournaments/{tournamentId}/groups/{groupId}", ["PATCH"], () =>
    Results.StatusCode(StatusCodes.Status501NotImplemented));

app.MapMethods("/tournaments/{tournamentId}/groups/{groupId}/teams", ["PATCH"], async (
    string tournamentId,
    string groupId,
    List<Team> teams,
    GroupDelegate groups,
    CancellationToken cancellationToken) =>
{
    var result = await groups.AddTeamsAsync(tournamentId, groupId, teams, cancellationToken);
    return result.IsSuccess
        ? Results.NoContent()
        : Results.Text(result.Error, statusCode: StatusCodes.Status422UnprocessableEntity);
});

app.MapGet("/tournaments/{tournamentId}/matches", async (
    string tournamentId,
    MatchStage? stage,
    MatchDelegate matches,
    CancellationToken cancellationToken) =>
    Results.Json(await matches.GetMatchesAsync(tournamentId, stage, cancellationToken)));

app.MapGet("/tournaments/{tournamentId}/matches/{matchId}", async (
    string tournamentId,
    string matchId,
    MatchDelegate matches,
    CancellationToken cancellationToken) =>
{
    var match = await matches.GetMatchAsync(tournamentId, matchId, cancellationToken);
    return match is null ? Results.NotFound() : Results.Json(match);
});

app.MapPut("/tournaments/{tournamentId}/matches/{matchId}/score", async (
    string tournamentId,
    string matchId,
    Score score,
    MatchDelegate matches,
    CancellationToken cancellationToken) =>
{
    var result = await matches.UpdateScoreAsync(tournamentId, matchId, score, cancellationToken);
    return result switch
    {
        { IsSuccess: true } => Results.NoContent(),
        { IsNotFound: true } => Results.Text(result.Error, statusCode: StatusCodes.Status404NotFound),
        _ => Results.Text(result.Error, statusCode: StatusCodes.Status422UnprocessableEntity)
    };
});

app.MapGet("/tournaments/{tournamentId}/standings", async (
    string tournamentId,
    MatchDelegate matches,
    CancellationToken cancellationToken) =>
{
    var standings = await matches.GetStandingsAsync(tournamentId, cancellationToken);
    return standings is null ? Results.NotFound() : Results.Json(standings);
});

app.Run();

public partial class Program;
