//
// Created by root on 9/27/25.
//

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"
#include "exception/Duplicate.hpp"


TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate) : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if(auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
        nlohmann::json body = team;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header("Content-Type", "application/json");
        return response;
    }
    return crow::response{crow::NOT_FOUND, "team not found"};
}

crow::response TeamController::getAllTeams() const {

    nlohmann::json body = teamDelegate->GetAllTeams();
    return crow::response{200, body.dump()};
}

crow::response TeamController::CreateTeam(const crow::request& request) const {
    crow::response response;

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Team team = requestBody;

    try {
    auto createdId = teamDelegate->CreateTeam(team);
    response.code = crow::CREATED;
    response.add_header("location", createdId.data());
    } catch (const DuplicateException& e) {
        response.code = crow::CONFLICT;
        response.body = e.what();
    } catch (const std::exception& e) {
        response.code = crow::INTERNAL_SERVER_ERROR;
        response.body = "Internal server error";
    }
    
    return response;

}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, CreateTeam, "/teams", "POST"_method)

