//
// Created by edgar on 11/10/25.
//

#include "controller/MatchController.hpp"

#include "configuration/RouteDefinition.hpp"

using nlohmann::json;

static constexpr char JSON_CONTENT_TYPE[]   = "application/json";
static constexpr char CONTENT_TYPE_HEADER[] = "content-type";

const std::regex MatchController::kIdPattern{R"(^[A-Za-z0-9_-]+$)"};

MatchController::MatchController(const std::shared_ptr<IMatchDelegate>& d)
    : delegate(d)
{
}

// GET /tournaments/<TOURNAMENT_ID>/matches[?showMatches=played|pending]
crow::response
MatchController::GetMatches(const crow::request& req,
                            const std::string& tournamentId) const
{
    if (!std::regex_match(tournamentId, kIdPattern))
        return crow::response{crow::BAD_REQUEST, "Invalid tournament id"};

    std::optional<std::string> filter;
    if (auto it = req.url_params.get("showMatches"))
    {
        std::string value{it};
        if (value == "played" || value == "pending")
        {
            filter = value;
        }
    }

    auto r = delegate->GetMatches(tournamentId, filter);
    if (!r.has_value())
    {
        const auto& err = r.error();
        if (err.rfind("db-error", 0) == 0)
            return crow::response{crow::INTERNAL_SERVER_ERROR, err};
        if (err == "tournament-not-found")
            return crow::response{crow::NOT_FOUND, "tournament not found"};
        return crow::response{crow::INTERNAL_SERVER_ERROR, err};
    }

    const auto& docs = r.value();
    json arr = json::array();
    for (const auto& d : docs)
        arr.push_back(d);

    crow::response res{crow::OK, arr.dump()};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return res;
}

// GET /tournaments/<TOURNAMENT_ID>/matches/<MATCH_ID>
crow::response
MatchController::GetMatch(const std::string& tournamentId,
                          const std::string& matchId) const
{
    if (!std::regex_match(tournamentId, kIdPattern) ||
        !std::regex_match(matchId, kIdPattern))
    {
        return crow::response{crow::BAD_REQUEST, "Invalid id format"};
    }

    auto r = delegate->GetMatch(tournamentId, matchId);
    if (!r.has_value())
    {
        const auto& err = r.error();
        if (err == "match-not-found")
            return crow::response{crow::NOT_FOUND, "match not found"};
        if (err.rfind("db-error", 0) == 0)
            return crow::response{crow::INTERNAL_SERVER_ERROR, err};
        return crow::response{crow::INTERNAL_SERVER_ERROR, err};
    }

    crow::response res{crow::OK, r.value().dump()};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return res;
}

// PATCH /tournaments/<TOURNAMENT_ID>/matches/<MATCH_ID>
crow::response
MatchController::PatchScore(const crow::request& req,
                            const std::string& tournamentId,
                            const std::string& matchId) const
{
    if (!std::regex_match(tournamentId, kIdPattern) ||
        !std::regex_match(matchId, kIdPattern))
    {
        return crow::response{crow::BAD_REQUEST, "Invalid id format"};
    }

    if (!json::accept(req.body))
        return crow::response{crow::BAD_REQUEST, "invalid json"};

    json body = json::parse(req.body);
    if (!body.contains("score") || !body["score"].is_object())
        return crow::response{crow::BAD_REQUEST, "field 'score' is required"};

    const auto& score = body["score"];
    if (!score.contains("home") || !score.contains("visitor") ||
        !score["home"].is_number_integer() ||
        !score["visitor"].is_number_integer())
    {
        return crow::response{crow::BAD_REQUEST, "invalid score format"};
    }

    int home    = score["home"].get<int>();
    int visitor = score["visitor"].get<int>();

    auto r = delegate->UpdateScore(tournamentId, matchId, home, visitor);
    if (!r.has_value())
    {
        const auto& err = r.error();
        if (err == "match-not-found")
            return crow::response{crow::NOT_FOUND, "match not found"};
        if (err == "invalid-score")
            return crow::response{422, "invalid score"};
        if (err.rfind("db-error", 0) == 0)
            return crow::response{crow::INTERNAL_SERVER_ERROR, err};
        return crow::response{crow::INTERNAL_SERVER_ERROR, err};
    }

    return crow::response{crow::NO_CONTENT};
}

// Registro de rutas (igual que los otros controllers)
REGISTER_ROUTE(MatchController, GetMatches,
               "/tournaments//matches", "GET"_method)

REGISTER_ROUTE(MatchController, GetMatch,
               "/tournaments//matches/", "GET"_method)

REGISTER_ROUTE(MatchController, PatchScore,
               "/tournaments//matches/", "PATCH"_method)
