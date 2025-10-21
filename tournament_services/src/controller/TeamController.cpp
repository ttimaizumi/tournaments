//
// Created by root on 9/27/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate) : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    auto result = teamDelegate->GetTeam(teamId);
    if(!result.has_value()) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    auto team = result.value();
    if(team == nullptr) {
        return crow::response{crow::NOT_FOUND, "team not found"};
    }

    nlohmann::json body = team;
    auto response = crow::response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response TeamController::getAllTeams() const {
    auto result = teamDelegate->GetAllTeams();
    if(!result.has_value()) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    nlohmann::json body = result.value();
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response TeamController::SaveTeam(const crow::request& request) const {
    crow::response response;

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Team team = requestBody;

    auto result = teamDelegate->SaveTeam(team);

    if(!result.has_value()) {
        const std::string& error = result.error();
        if(error.find("already exists") != std::string::npos) {
            response.code = crow::CONFLICT;
        } else {
            response.code = crow::INTERNAL_SERVER_ERROR;
            response.body = error;
        }
        return response;
    }

    response.code = crow::CREATED;
    response.add_header("location", result.value());

    return response;
}

crow::response TeamController::UpdateTeam(const crow::request& request, const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
    }

    nlohmann::json body = nlohmann::json::parse(request.body);
    domain::Team team = body;
    team.Id = teamId;

    auto result = teamDelegate->UpdateTeam(team);

    if(!result.has_value()) {
        const std::string& error = result.error();
        if(error.find("not found") != std::string::npos) {
            return crow::response{crow::NOT_FOUND};
        } else {
            return crow::response{crow::INTERNAL_SERVER_ERROR, error};
        }
    }

    return crow::response{crow::NO_CONTENT};
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PATCH"_method)