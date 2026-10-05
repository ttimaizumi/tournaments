using Npgsql;
using Tournaments.Core;
using Tournaments.Infrastructure;
using Tournaments.Consumer;

var builder = Host.CreateApplicationBuilder(args);
builder.Services.AddSingleton(_ => NpgsqlDataSource.Create(
    builder.Configuration.GetConnectionString("Tournaments")
    ?? throw new InvalidOperationException("ConnectionStrings:Tournaments is required.")));
builder.Services.AddSingleton<IGroupRepository, GroupRepository>();
builder.Services.AddHostedService<TeamAddedWorker>();

await builder.Build().RunAsync();
