using System.Text.Json;
using System.Text.Json.Serialization;

namespace Tournaments.Infrastructure;

internal static class DatabaseJson
{
    public static readonly JsonSerializerOptions Options = new(JsonSerializerDefaults.Web)
    {
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
    };

    public static string Serialize<T>(T value) => JsonSerializer.Serialize(value, Options);

    public static T Deserialize<T>(string value) =>
        JsonSerializer.Deserialize<T>(value, Options) ?? throw new InvalidOperationException("Database document was null.");
}
