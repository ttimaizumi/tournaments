using System.Net;
using System.Net.Http.Json;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;
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
