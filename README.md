# Tournaments

The project is a .NET 8 solution containing an ASP.NET Core API, PostgreSQL repositories, and an ActiveMQ background consumer.

## Projects

- `Tournaments.Core`: domain models, contracts, and application services.
- `Tournaments.Infrastructure`: Npgsql repositories and ActiveMQ publishing.
- `Tournaments.Api`: HTTP endpoints on port 8080.
- `Tournaments.Consumer`: `tournament.team-add` queue consumer.
- `Tournaments.Tests`: API contract and application tests.

## Local development

Requires the .NET 8 SDK.

```bash
dotnet restore
dotnet test
dotnet run --project src/Tournaments.Api
```

Configuration can be overridden with standard ASP.NET Core environment variables, for example:

```bash
ConnectionStrings__Tournaments='Host=localhost;Database=tournament_db;Username=tournament_svc;Password=password' \
ActiveMq__BrokerUri='activemq:tcp://localhost:61616' \
dotnet run --project src/Tournaments.Api
```

## Containers

Start PostgreSQL, ActiveMQ, the API, and the consumer with:

```bash
podman compose up --build
```

The API is available at `http://localhost:8080`; the ActiveMQ console is available at `http://localhost:8161`.
