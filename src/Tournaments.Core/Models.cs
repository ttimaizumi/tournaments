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
    public int NumberOfGroups { get; set; } = 1;
    public int MaxTeamsPerGroup { get; set; } = 16;
    public TournamentType Type { get; set; } = TournamentType.ROUND_ROBIN;
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

public enum Winner
{
    HOME,
    VISITOR
}

public sealed class Score
{
    public int HomeTeamScore { get; set; }
    public int VisitorTeamScore { get; set; }

    public Winner GetWinner() => HomeTeamScore > VisitorTeamScore ? Winner.HOME : Winner.VISITOR;
}

public sealed class Match
{
    public string HomeTeamId { get; set; } = string.Empty;
    public string VisitorTeamId { get; set; } = string.Empty;
    public Score Score { get; set; } = new();
}

public sealed class TeamAddEvent
{
    public string TournamentId { get; set; } = string.Empty;
    public string GroupId { get; set; } = string.Empty;
    public string TeamId { get; set; } = string.Empty;
}
