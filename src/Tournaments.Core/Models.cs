using System.Text.Json.Serialization;

namespace Tournaments.Core;

public sealed class Team
{
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public string? Id { get; set; }

    public string Name { get; set; } = string.Empty;
}

[JsonConverter(typeof(JsonStringEnumConverter<TournamentType>))]
public enum TournamentType
{
    ROUND_ROBIN,
    NFL
}

public sealed class TournamentFormat
{
    public const int RequiredNumberOfGroups = 1;
    public const int RequiredTeamsPerGroup = 32;
    public const int DefaultPlayoffTeams = 8;

    public int NumberOfGroups { get; set; } = RequiredNumberOfGroups;
    public int MaxTeamsPerGroup { get; set; } = RequiredTeamsPerGroup;
    public TournamentType Type { get; set; } = TournamentType.ROUND_ROBIN;

    /// <summary>Top teams from the round robin that advance to a single-elimination playoff.</summary>
    public int PlayoffTeams { get; set; } = DefaultPlayoffTeams;

    public string? Validate()
    {
        if (NumberOfGroups != RequiredNumberOfGroups)
        {
            return $"Tournaments must have exactly {RequiredNumberOfGroups} group";
        }

        if (MaxTeamsPerGroup != RequiredTeamsPerGroup)
        {
            return $"Groups must have exactly {RequiredTeamsPerGroup} teams";
        }

        if (PlayoffTeams < 2 || PlayoffTeams > MaxTeamsPerGroup || (PlayoffTeams & (PlayoffTeams - 1)) != 0)
        {
            return $"Playoff teams must be a power of two between 2 and {MaxTeamsPerGroup}";
        }

        return null;
    }
}

public sealed class Tournament
{
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public string? Id { get; set; }

    public string Name { get; set; } = string.Empty;
    public TournamentFormat Format { get; set; } = new();
}

public sealed class Group
{
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public string? Id { get; set; }

    public string Name { get; set; } = string.Empty;
    public string TournamentId { get; set; } = string.Empty;
    public List<Team> Teams { get; set; } = [];
}

[JsonConverter(typeof(JsonStringEnumConverter<Winner>))]
public enum Winner
{
    HOME,
    VISITOR,
    DRAW
}

public sealed class Score
{
    public int HomeTeamScore { get; set; }
    public int VisitorTeamScore { get; set; }

    public Winner GetWinner() =>
        HomeTeamScore > VisitorTeamScore ? Winner.HOME
        : HomeTeamScore < VisitorTeamScore ? Winner.VISITOR
        : Winner.DRAW;
}

[JsonConverter(typeof(JsonStringEnumConverter<MatchStage>))]
public enum MatchStage
{
    ROUND_ROBIN,
    PLAYOFF
}

public sealed class Match
{
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public string? Id { get; set; }

    public string TournamentId { get; set; } = string.Empty;
    public string GroupId { get; set; } = string.Empty;
    public MatchStage Stage { get; set; }

    /// <summary>1-based round within the stage.</summary>
    public int Round { get; set; }

    /// <summary>1-based position within the round; for playoffs, the bracket slot.</summary>
    public int MatchNumber { get; set; }

    public string HomeTeamId { get; set; } = string.Empty;
    public string VisitorTeamId { get; set; } = string.Empty;

    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public int? HomeSeed { get; set; }

    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public int? VisitorSeed { get; set; }

    /// <summary>Null until the match has been played.</summary>
    public Score? Score { get; set; }

    [JsonIgnore]
    public bool IsPlayed => Score is not null;

    public string? GetWinnerTeamId() => Score?.GetWinner() switch
    {
        Winner.HOME => HomeTeamId,
        Winner.VISITOR => VisitorTeamId,
        _ => null
    };
}

public sealed class Standing
{
    public int Rank { get; set; }
    public string TeamId { get; set; } = string.Empty;
    public string TeamName { get; set; } = string.Empty;
    public int Played { get; set; }
    public int Wins { get; set; }
    public int Draws { get; set; }
    public int Losses { get; set; }
    public int ScoreFor { get; set; }
    public int ScoreAgainst { get; set; }
    public int ScoreDifference => ScoreFor - ScoreAgainst;
    public int Points => Wins * 3 + Draws;
}

public sealed class TeamAddEvent
{
    public string TournamentId { get; set; } = string.Empty;
    public string GroupId { get; set; } = string.Empty;
    public string TeamId { get; set; } = string.Empty;
}
