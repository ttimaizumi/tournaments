using System.Diagnostics;
using System.Net;
using System.Net.Http.Json;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;
using Npgsql;
using OpenTelemetry.Exporter;
using Tournaments.Core;

namespace Tournaments.Tests;

public sealed class ApiTests : IClassFixture<TournamentApiFactory>
{
    private readonly TournamentApiFactory factory;

    public ApiTests(TournamentApiFactory factory) => this.factory = factory;

    [Fact]
    public async Task Health_reports_running()
    {
        using var client = factory.CreateClient();
        var response = await client.GetAsync("/health");

        Assert.Equal(HttpStatusCode.OK, response.StatusCode);
        Assert.Equal("Services running", await response.Content.ReadAsStringAsync());
    }

    [Fact]
    public async Task Metrics_exports_http_request_histogram()
    {
        using var client = factory.CreateClient();
        await client.GetAsync("/teams");

        var response = await client.GetAsync("/metrics");
        var metrics = await response.Content.ReadAsStringAsync();

        Assert.Equal(HttpStatusCode.OK, response.StatusCode);
        Assert.Contains("http_server_request_duration_seconds_bucket", metrics);
        Assert.Contains("http_route=\"/teams\"", metrics);
    }

    [Fact]
    public async Task Metrics_exports_named_database_pool_without_connection_details()
    {
        using var client = factory.CreateClient();
        _ = factory.Services.GetRequiredService<NpgsqlDataSource>();

        var metrics = await client.GetStringAsync("/metrics");

        Assert.Contains("db_client_connection_max", metrics);
        Assert.Contains("db_client_connection_pool_name=\"tournament-db\"", metrics);
        Assert.DoesNotContain("Password=", metrics, StringComparison.OrdinalIgnoreCase);
        Assert.DoesNotContain("Host=tournament_db", metrics, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void Database_activity_source_is_enabled_for_tracing()
    {
        using var client = factory.CreateClient();
        using var source = new ActivitySource("Npgsql");
        using var activity = source.StartActivity("database tracing registration", ActivityKind.Client);

        Assert.NotNull(activity);
        Assert.True(activity.IsAllDataRequested);
    }

    [Fact]
    public async Task Get_team_rejects_invalid_identifier()
    {
        using var client = factory.CreateClient();
        var response = await client.GetAsync("/teams/not_valid!");

        Assert.Equal(HttpStatusCode.BadRequest, response.StatusCode);
        Assert.Equal("Invalid ID format", await response.Content.ReadAsStringAsync());
    }

    [Fact]
    public async Task Create_team_returns_raw_identifier_location()
    {
        using var client = factory.CreateClient();
        var response = await client.PostAsJsonAsync("/teams", new Team { Name = "Falcons" });

        Assert.Equal(HttpStatusCode.Created, response.StatusCode);
        Assert.True(Guid.TryParse(response.Headers.Location?.OriginalString, out _));
    }
}

public sealed class TournamentApiFactory : WebApplicationFactory<Program>
{
    protected override void ConfigureWebHost(IWebHostBuilder builder)
    {
        builder.ConfigureServices(services =>
        {
            services.Configure<PrometheusAspNetCoreOptions>(options =>
                options.ScrapeResponseCacheDurationMilliseconds = 0);
            services.RemoveAll<ITeamRepository>();
            services.RemoveAll<ITournamentRepository>();
            services.RemoveAll<IGroupRepository>();
            services.RemoveAll<IMessagePublisher>();
            services.AddSingleton<ITeamRepository, FakeTeamRepository>();
            services.AddSingleton<ITournamentRepository, FakeTournamentRepository>();
            services.AddSingleton<IGroupRepository, FakeGroupRepository>();
            services.AddSingleton<IMessagePublisher, FakeMessagePublisher>();
        });
    }
}
