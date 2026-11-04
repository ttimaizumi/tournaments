using Npgsql;
using Tournaments.Core;
using Tournaments.Infrastructure;
using Tournaments.Consumer;

var builder = Host.CreateApplicationBuilder(args);
builder.Services.AddSingleton(_ => NpgsqlDataSource.Create(
    builder.Configuration.GetConnectionString("Tournaments")
    ?? throw new InvalidOperationException("ConnectionStrings:Tournaments is required.")));
builder.Services.AddSingleton<ITournamentRepository, TournamentRepository>();
builder.Services.AddSingleton<IGroupRepository, GroupRepository>();
builder.Services.AddSingleton<IMatchRepository, MatchRepository>();
builder.Services.AddSingleton<MatchDelegate>();
builder.Services.AddHostedService<TeamAddedWorker>();

await builder.Build().RunAsync();
