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

    public async Task<bool> AddTeamsAsync(
        string groupId,
        IReadOnlyList<Team> teams,
        int maxTeams,
        CancellationToken cancellationToken = default)
    {
        // The capacity and uniqueness predicates are re-evaluated against the latest row version when
        // a concurrent UPDATE commits first, so two requests can't jointly overfill the group.
        const string sql = """
            UPDATE groups
            SET document = jsonb_set(document, '{teams}', COALESCE(document->'teams', '[]'::jsonb) || @teams::jsonb),
                last_update_date = CURRENT_TIMESTAMP
            WHERE id = @groupId
              AND jsonb_array_length(COALESCE(document->'teams', '[]'::jsonb)) + jsonb_array_length(@teams::jsonb) <= @maxTeams
              AND NOT EXISTS (
                  SELECT 1
                  FROM jsonb_array_elements(COALESCE(document->'teams', '[]'::jsonb)) AS existing(team)
                  WHERE existing.team->>'id' IN (SELECT added->>'id' FROM jsonb_array_elements(@teams::jsonb) AS added))
            """;
        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("groupId", Guid.Parse(groupId));
        command.Parameters.AddWithValue("teams", NpgsqlDbType.Jsonb, DatabaseJson.Serialize(teams));
        command.Parameters.AddWithValue("maxTeams", maxTeams);
        return await command.ExecuteNonQueryAsync(cancellationToken) == 1;
    }

    private static Group ReadGroup(NpgsqlDataReader reader)
    {
        var group = DatabaseJson.Deserialize<Group>(reader.GetString(2));
        group.Id = reader.GetGuid(0).ToString();
        group.TournamentId = reader.GetGuid(1).ToString();
        return group;
    }
}

public sealed class MatchRepository(NpgsqlDataSource dataSource) : IMatchRepository
{
    private const string Columns = "id, tournament_id, document::text";

    public async Task<Match?> GetAsync(string tournamentId, string matchId, CancellationToken cancellationToken = default)
    {
        const string sql = $"SELECT {Columns} FROM matches WHERE tournament_id = @tournamentId AND id = @matchId";
        if (!Guid.TryParse(tournamentId, out var parsedTournamentId) || !Guid.TryParse(matchId, out var parsedMatchId))
        {
            return null;
        }

        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("tournamentId", parsedTournamentId);
        command.Parameters.AddWithValue("matchId", parsedMatchId);
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        return await reader.ReadAsync(cancellationToken) ? ReadMatch(reader) : null;
    }

    public async Task<IReadOnlyList<Match>> GetAllAsync(string tournamentId, CancellationToken cancellationToken = default)
    {
        const string sql = $"""
            SELECT {Columns}
            FROM matches
            WHERE tournament_id = @tournamentId
            ORDER BY document->>'stage' = 'PLAYOFF', (document->>'round')::int, (document->>'matchNumber')::int
            """;
        if (!Guid.TryParse(tournamentId, out var parsedTournamentId))
        {
            return [];
        }

        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("tournamentId", parsedTournamentId);
        await using var reader = await command.ExecuteReaderAsync(cancellationToken);
        var matches = new List<Match>();
        while (await reader.ReadAsync(cancellationToken))
        {
            matches.Add(ReadMatch(reader));
        }

        return matches;
    }

    public async Task<int> CreateManyAsync(IReadOnlyList<Match> matches, CancellationToken cancellationToken = default)
    {
        if (matches.Count == 0)
        {
            return 0;
        }

        var tournamentId = matches[0].TournamentId;
        if (matches.Any(match => match.TournamentId != tournamentId))
        {
            throw new ArgumentException("All matches must belong to one tournament.", nameof(matches));
        }

        // One statement, so the whole schedule lands or none of it does; the unique slot index
        // turns a duplicate generation (redelivered message, concurrent score update) into a no-op.
        const string sql = """
            INSERT INTO matches(tournament_id, document)
            SELECT @tournamentId, match.document
            FROM jsonb_array_elements(@documents::jsonb) AS match(document)
            ON CONFLICT DO NOTHING
            """;
        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("tournamentId", Guid.Parse(tournamentId));
        command.Parameters.AddWithValue("documents", NpgsqlDbType.Jsonb, DatabaseJson.Serialize(matches));
        return await command.ExecuteNonQueryAsync(cancellationToken);
    }

    public async Task<bool> UpdateScoreAsync(
        string tournamentId,
        string matchId,
        Score score,
        CancellationToken cancellationToken = default)
    {
        const string sql = """
            UPDATE matches
            SET document = jsonb_set(document, '{score}', @score::jsonb),
                last_update_date = CURRENT_TIMESTAMP
            WHERE tournament_id = @tournamentId AND id = @matchId
            """;
        if (!Guid.TryParse(tournamentId, out var parsedTournamentId) || !Guid.TryParse(matchId, out var parsedMatchId))
        {
            return false;
        }

        await using var command = dataSource.CreateCommand(sql);
        command.Parameters.AddWithValue("tournamentId", parsedTournamentId);
        command.Parameters.AddWithValue("matchId", parsedMatchId);
        command.Parameters.AddWithValue("score", NpgsqlDbType.Jsonb, DatabaseJson.Serialize(score));
        return await command.ExecuteNonQueryAsync(cancellationToken) == 1;
    }

    private static Match ReadMatch(NpgsqlDataReader reader)
    {
        var match = DatabaseJson.Deserialize<Match>(reader.GetString(2));
        match.Id = reader.GetGuid(0).ToString();
        match.TournamentId = reader.GetGuid(1).ToString();
        return match;
    }
}
