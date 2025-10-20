//
// Created by root on 10/8/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/GroupController.hpp"
#include "domain/Utilities.hpp"

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate) : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController()
{
}

crow::response GroupController::GetGroups(const std::string& tournamentId) {
    if (auto groups = groupDelegate->GetGroups(tournamentId)) {
        nlohmann::json body = *groups;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{422};
}

crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId) {
    try {
        auto requestBody = nlohmann::json::parse(request.body);
        domain::Group group = requestBody;

        auto groupId = groupDelegate->CreateGroup(tournamentId, group);
        crow::response response;
        response.code = groupId ? crow::CREATED : 422;
        if (groupId) response.add_header("location", *groupId);
        return response;
    } catch (const nlohmann::json::exception&) {
        return crow::response{422};
    }
}

crow::response GroupController::UpdateTeams(
        const crow::request& request,
        const std::string& tournamentId,
        const std::string& groupId
) {
    try {
        domain::Team team = nlohmann::json::parse(request.body).get<domain::Team>();
        std::vector<domain::Team> teams = { team };

        const auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
        crow::response response;

        if (result.has_value()) {
            response.code = crow::status::CREATED;
        } else {
            response.code = 422;
            response.body = result.error();
        }

        return response;

    } catch (const nlohmann::json::exception& e) {
        crow::response res{422};
        res.body = std::string("JSON parse error: ") + e.what();
        return res;
    }
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "POST"_method)
