using Npgsql;
using NpgsqlTypes;
using Tournaments.Core;

namespace Tournaments.Infrastructure;

public sealed class TeamRepository(NpgsqlDataSource dataSource) : ITeamRepository
{
    public async Task<Team?> GetAsync(string id, CancellationToken cancellationToken = default)
    {
        const string sql = "SELECT id, document::text FROM teams WHERE id = @id";
        await using var command = dataSource.CreateCommand(sql);
        if (!Guid.TryParse(id, out var parsedId))
        {
            return null;
        }

        command.Parameters.AddWithValue("id", parsedId);
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        if (!await reader.ReadAsync(cancellationToken))
        {
            return null;
        }

        return ReadTeam(reader);
    }

    public async Task<IReadOnlyList<Team>> GetAllAsync(CancellationToken cancellationToken = default)
    {
        const string sql = "SELECT id, document::text FROM teams ORDER BY created_at";
        await using var command = dataSource.CreateCommand(sql);
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        var teams = new List<Team>();
        while (await reader.ReadAsync(cancellationToken))
        {
            teams.Add(ReadTeam(reader));
        }

        return teams;
    }

    public async Task<string> CreateAsync(Team team, CancellationToken cancellationToken = default)
    {
        const string sql = "INSERT INTO teams(document) VALUES (@document) RETURNING id";
        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("document", NpgsqlDbType.Jsonb, DatabaseJson.Serialize(team));
        var id = (Guid)(await command.ExecuteScalarAsync(cancellationToken)
            ?? throw new InvalidOperationException("Team insert did not return an ID."));
        return id.ToString();
    }

    private static Team ReadTeam(NpgsqlDataReader reader)
    {
        var team = DatabaseJson.Deserialize<Team>(reader.GetString(1));
        team.Id = reader.GetGuid(0).ToString();
        return team;
    }
}

public sealed class TournamentRepository(NpgsqlDataSource dataSource) : ITournamentRepository
{
    public async Task<Tournament?> GetAsync(string id, CancellationToken cancellationToken = default)
    {
        const string sql = "SELECT id, document::text FROM tournaments WHERE id = @id";
        if (!Guid.TryParse(id, out var parsedId))
        {
            return null;
        }

        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("id", parsedId);
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        return await reader.ReadAsync(cancellationToken) ? ReadTournament(reader) : null;
    }

    public async Task<IReadOnlyList<Tournament>> GetAllAsync(CancellationToken cancellationToken = default)
    {
        const string sql = "SELECT id, document::text FROM tournaments ORDER BY created_at";
        await using var command = dataSource.CreateCommand(sql);
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        var tournaments = new List<Tournament>();
        while (await reader.ReadAsync(cancellationToken))
        {
            tournaments.Add(ReadTournament(reader));
        }

        return tournaments;
    }

    public async Task<string> CreateAsync(Tournament tournament, CancellationToken cancellationToken = default)
    {
        const string sql = "INSERT INTO tournaments(document) VALUES (@document) RETURNING id";
        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("document", NpgsqlDbType.Jsonb, DatabaseJson.Serialize(tournament));
        var id = (Guid)(await command.ExecuteScalarAsync(cancellationToken)
            ?? throw new InvalidOperationException("Tournament insert did not return an ID."));
        return id.ToString();
    }

    private static Tournament ReadTournament(NpgsqlDataReader reader)
    {
        var tournament = DatabaseJson.Deserialize<Tournament>(reader.GetString(1));
        tournament.Id = reader.GetGuid(0).ToString();
        return tournament;
    }
}

public sealed class GroupRepository(NpgsqlDataSource dataSource) : IGroupRepository
{
    public async Task<Group?> GetAsync(
        string tournamentId,
        string groupId,
        CancellationToken cancellationToken = default)
    {
        const string sql = "SELECT id, tournament_id, document::text FROM groups WHERE tournament_id = @tournamentId AND id = @groupId";
        if (!Guid.TryParse(tournamentId, out var parsedTournamentId) || !Guid.TryParse(groupId, out var parsedGroupId))
        {
            return null;
        }

        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("tournamentId", parsedTournamentId);
        command.Parameters.AddWithValue("groupId", parsedGroupId);
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        return await reader.ReadAsync(cancellationToken) ? ReadGroup(reader) : null;
    }

    public async Task<Group?> FindByTeamAsync(
        string tournamentId,
        string teamId,
        CancellationToken cancellationToken = default)
    {
        const string sql = "SELECT id, tournament_id, document::text FROM groups WHERE tournament_id = @tournamentId AND document->'teams' @> @team::jsonb LIMIT 1";
        if (!Guid.TryParse(tournamentId, out var parsedTournamentId))
        {
            return null;
        }

        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("tournamentId", parsedTournamentId);
        command.Parameters.AddWithValue("team", NpgsqlDbType.Jsonb, $"[{{\"id\":\"{teamId}\"}}]");
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        return await reader.ReadAsync(cancellationToken) ? ReadGroup(reader) : null;
    }

    public async Task<IReadOnlyList<Group>> GetAllAsync(
        string tournamentId,
        CancellationToken cancellationToken = default)
    {
        const string sql = "SELECT id, tournament_id, document::text FROM groups WHERE tournament_id = @tournamentId ORDER BY created_at";
        if (!Guid.TryParse(tournamentId, out var parsedTournamentId))
        {
            return [];
        }

        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("tournamentId", parsedTournamentId);
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        var groups = new List<Group>();
        while (await reader.ReadAsync(cancellationToken))
        {
            groups.Add(ReadGroup(reader));
        }

        return groups;
    }

    public async Task<string> CreateAsync(Group group, CancellationToken cancellationToken = default)
    {
        const string sql = "INSERT INTO groups(tournament_id, document) VALUES (@tournamentId, @document) RETURNING id";
        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("tournamentId", Guid.Parse(group.TournamentId));
        command.Parameters.AddWithValue("document", NpgsqlDbType.Jsonb, DatabaseJson.Serialize(group));
        var id = (Guid)(await command.ExecuteScalarAsync(cancellationToken)
            ?? throw new InvalidOperationException("Group insert did not return an ID."));
        return id.ToString();
    }

    public async Task AddTeamAsync(string groupId, Team team, CancellationToken cancellationToken = default)
    {
        const string sql = """
            UPDATE groups
            SET document = jsonb_set(document, '{teams}', COALESCE(document->'teams', '[]'::jsonb) || @team::jsonb),
                last_update_date = CURRENT_TIMESTAMP
            WHERE id = @groupId
            """;
        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("groupId", Guid.Parse(groupId));
        command.Parameters.AddWithValue("team", NpgsqlDbType.Jsonb, DatabaseJson.Serialize(team));
        if (await command.ExecuteNonQueryAsync(cancellationToken) == 0)
        {
            throw new InvalidOperationException("Group no longer exists.");
        }
    }

    private static Group ReadGroup(NpgsqlDataReader reader)
    {
        var group = DatabaseJson.Deserialize<Group>(reader.GetString(2));
        group.Id = reader.GetGuid(0).ToString();
        group.TournamentId = reader.GetGuid(1).ToString();
        return group;
    }
}
