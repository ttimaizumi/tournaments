#include "controller/MatchController.hpp"
#include "configuration/RouteDefinition.hpp"

using nlohmann::json;

static constexpr char JSON_CONTENT_TYPE[]   = "application/json";
static constexpr char CONTENT_TYPE_HEADER[] = "content-type";

const std::regex MatchController::kIdPattern{R"(^[A-Za-z0-9_-]+$)"};

crow::response
MatchController::CreateMatch(const crow::request& req,
                             const std::string& tournamentId) const {
    if (!nlohmann::json::accept(req.body)) {
        return crow::response{crow::BAD_REQUEST, "invalid json"};
    }
    auto body = json::parse(req.body);

    auto r = delegate->CreateMatch(tournamentId, body);
    if (!r.has_value()) {
        const auto& err = r.error();
        if (err == "invalid-body") return crow::response{crow::BAD_REQUEST, "invalid body"};
        if (err.rfind("db-error", 0) == 0) return crow::response{crow::INTERNAL_SERVER_ERROR, err};
        return crow::response{crow::INTERNAL_SERVER_ERROR, err};
    }

    crow::response res{crow::CREATED};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    res.write(r.value().dump());
    return res;
}

crow::response
MatchController::GetMatches(const crow::request& req,
                            const std::string& tournamentId) const {
    std::optional<std::string> status;
    if (auto s = req.url_params.get("status")) status = std::string{s};

    auto r = delegate->GetMatches(tournamentId, status);
    if (!r.has_value()) {
        const auto& err = r.error();
        return crow::response{crow::INTERNAL_SERVER_ERROR, err};
    }

    crow::response res{crow::OK};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    res.write(nlohmann::json{r.value()}.dump());
    return res;
}

crow::response
MatchController::GetMatch(const std::string& tournamentId,
                          const std::string& matchId) const {
    if (!std::regex_match(matchId, kIdPattern)) {
        return crow::response{crow::BAD_REQUEST, "invalid id"};
    }
    auto r = delegate->GetMatch(tournamentId, matchId);
    if (!r.has_value()) {
        const auto& err = r.error();
        if (err == "not-found") return crow::response{crow::NOT_FOUND, "not found"};
        return crow::response{crow::INTERNAL_SERVER_ERROR, err};
    }

    crow::response res{crow::OK};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    res.write(r.value().dump());
    return res;
}

crow::response
MatchController::PatchScore(const crow::request& req,
                            const std::string& tournamentId,
                            const std::string& matchId) const {
    if (!nlohmann::json::accept(req.body)) {
        return crow::response{crow::BAD_REQUEST, "invalid json"};
    }

    auto body = json::parse(req.body);
    if (!body.contains("score") || !body["score"].is_object()) {
        return crow::response{crow::BAD_REQUEST, "missing score"};
    }
    const auto& score = body["score"];
    if (!score.contains("home") || !score.contains("visitor") ||
        !score["home"].is_number_integer() || !score["visitor"].is_number_integer()) {
        return crow::response{crow::BAD_REQUEST, "invalid score format"};
    }

    int home    = score["home"].get<int>();
    int visitor = score["visitor"].get<int>();

    auto r = delegate->UpdateScore(tournamentId, matchId, home, visitor);
    if (!r.has_value()) {
        const auto& err = r.error();
        if (err == "match-not-found") return crow::response{crow::NOT_FOUND, "match not found"};
        if (err == "invalid-score")   return crow::response{422, "invalid score"};
        if (err.rfind("db-error", 0) == 0) return crow::response{crow::INTERNAL_SERVER_ERROR, err};
        return crow::response{crow::INTERNAL_SERVER_ERROR, err};
    }

    return crow::response{crow::NO_CONTENT};
}

// Registro de rutas (usar <string> para inyectar IDs en la lambda)
REGISTER_ROUTE(MatchController, CreateMatch,
               "/tournaments/<string>/matches", "POST"_method)

REGISTER_ROUTE(MatchController, GetMatches,
               "/tournaments/<string>/matches", "GET"_method)

REGISTER_ROUTE(MatchController, GetMatch,
               "/tournaments/<string>/matches/<string>", "GET"_method)

REGISTER_ROUTE(MatchController, PatchScore,
               "/tournaments/<string>/matches/<string>", "PATCH"_method)

