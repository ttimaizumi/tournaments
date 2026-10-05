using Npgsql;
using Tournaments.Core;
using Tournaments.Infrastructure;

var builder = WebApplication.CreateBuilder(args);

builder.Services.ConfigureHttpJsonOptions(options =>
    options.SerializerOptions.DefaultIgnoreCondition = System.Text.Json.Serialization.JsonIgnoreCondition.WhenWritingNull);

builder.Services.AddSingleton(_ => NpgsqlDataSource.Create(
    builder.Configuration.GetConnectionString("Tournaments")
    ?? throw new InvalidOperationException("ConnectionStrings:Tournaments is required.")));
builder.Services.AddScoped<ITeamRepository, TeamRepository>();
builder.Services.AddScoped<ITournamentRepository, TournamentRepository>();
builder.Services.AddScoped<IGroupRepository, GroupRepository>();
builder.Services.AddSingleton<IMessagePublisher>(_ => new ActiveMqMessagePublisher(
    builder.Configuration["ActiveMq:BrokerUri"]
    ?? throw new InvalidOperationException("ActiveMq:BrokerUri is required.")));
builder.Services.AddScoped<TournamentService>();
builder.Services.AddScoped<GroupService>();

var app = builder.Build();

app.MapGet("/health", () => Results.Text("Services running"));

app.MapGet("/teams", async (ITeamRepository teams, CancellationToken cancellationToken) =>
    Results.Json(await teams.GetAllAsync(cancellationToken)));

app.MapGet("/teams/{teamId}", async (string teamId, ITeamRepository teams, CancellationToken cancellationToken) =>
{
    if (!System.Text.RegularExpressions.Regex.IsMatch(teamId, "^[A-Za-z0-9-]+$"))
    {
        return Results.Text("Invalid ID format", statusCode: StatusCodes.Status400BadRequest);
    }

    var team = await teams.GetAsync(teamId, cancellationToken);
    return team is null
        ? Results.Text("team not found", statusCode: StatusCodes.Status404NotFound)
        : Results.Json(team);
});

app.MapPost("/teams", async (Team team, HttpContext context, ITeamRepository teams, CancellationToken cancellationToken) =>
{
    var id = await teams.CreateAsync(team, cancellationToken);
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
    var id = await tournaments.CreateAsync(tournament, cancellationToken);
    context.Response.Headers.Location = id;
    return Results.StatusCode(StatusCodes.Status201Created);
});

app.MapGet("/tournaments/{tournamentId}/groups", async (
    string tournamentId,
    GroupService groups,
    CancellationToken cancellationToken) => Results.Json(await groups.GetAllAsync(tournamentId, cancellationToken)));

app.MapGet("/tournaments/{tournamentId}/groups/{groupId}", async (
    string tournamentId,
    string groupId,
    GroupService groups,
    CancellationToken cancellationToken) =>
{
    var group = await groups.GetAsync(tournamentId, groupId, cancellationToken);
    return group is null ? Results.NotFound() : Results.Json(group);
});

app.MapPost("/tournaments/{tournamentId}/groups", async (
    string tournamentId,
    Group group,
    HttpContext context,
    GroupService groups,
    CancellationToken cancellationToken) =>
{
    var result = await groups.CreateAsync(tournamentId, group, cancellationToken);
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
    GroupService groups,
    CancellationToken cancellationToken) =>
{
    var result = await groups.AddTeamsAsync(tournamentId, groupId, teams, cancellationToken);
    return result.IsSuccess
        ? Results.NoContent()
        : Results.Text(result.Error, statusCode: StatusCodes.Status422UnprocessableEntity);
});

app.Run();

public partial class Program;
