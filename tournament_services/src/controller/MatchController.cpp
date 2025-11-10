#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/MatchController.hpp"
#include "domain/Utilities.hpp"

MatchController::MatchController(const std::shared_ptr<IMatchDelegate>& matchDelegate)
    : matchDelegate(matchDelegate) {}

crow::response MatchController::getMatches(const crow::request& request, const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format"};
    }

    // Get filter from query parameter
    std::string filter = "all";
    if (request.url_params.get("showMatches") != nullptr) {
        filter = request.url_params.get("showMatches");
    }

    auto result = matchDelegate->GetMatches(tournamentId, filter);
    if(!result.has_value()) {
        const std::string& error = result.error();
        if(error.find("not found") != std::string::npos || error.find("not exist") != std::string::npos) {
            return crow::response{crow::NOT_FOUND, error};
        }
        return crow::response{crow::INTERNAL_SERVER_ERROR, error};
    }

    nlohmann::json body = result.value();
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response MatchController::getMatch(const std::string& tournamentId, const std::string& matchId) const {
    if(!std::regex_match(tournamentId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format"};
    }

    if(!std::regex_match(matchId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid match ID format"};
    }

    auto result = matchDelegate->GetMatch(tournamentId, matchId);
    if(!result.has_value()) {
        const std::string& error = result.error();
        if(error.find("not found") != std::string::npos || error.find("not exist") != std::string::npos) {
            return crow::response{crow::NOT_FOUND, error};
        }
        return crow::response{crow::INTERNAL_SERVER_ERROR, error};
    }

    auto match = result.value();
    if(match == nullptr) {
        return crow::response{crow::NOT_FOUND, "Match not found"};
    }

    nlohmann::json body = match;
    auto response = crow::response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response MatchController::updateMatchScore(const crow::request& request, const std::string& tournamentId, const std::string& matchId) const {
    if (!std::regex_match(tournamentId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format"};
    }

    if (!std::regex_match(matchId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid match ID format"};
    }

    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
    }

    nlohmann::json body = nlohmann::json::parse(request.body);

    if (!body.contains("score")) {
        return crow::response{crow::BAD_REQUEST, "Missing score field"};
    }

    domain::Score score = body["score"].get<domain::Score>();

    auto result = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    if(!result.has_value()) {
        const std::string& error = result.error();
        if(error.find("not found") != std::string::npos || error.find("not exist") != std::string::npos) {
            return crow::response{crow::NOT_FOUND, error};
        } else if(error.find("not allowed") != std::string::npos ||
                  error.find("invalid") != std::string::npos ||
                  error.find("Tie") != std::string::npos ||
                  error.find("non-negative") != std::string::npos) {
            return crow::response{422, error};
        } else {
            return crow::response{crow::INTERNAL_SERVER_ERROR, error};
        }
    }

    return crow::response{crow::NO_CONTENT};
}

REGISTER_ROUTE(MatchController, getMatches, "/tournaments/<string>/matches", "GET"_method)
REGISTER_ROUTE(MatchController, getMatch, "/tournaments/<string>/matches/<string>", "GET"_method)
REGISTER_ROUTE(MatchController, updateMatchScore, "/tournaments/<string>/matches/<string>", "PATCH"_method)
