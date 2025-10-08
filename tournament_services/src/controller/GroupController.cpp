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

crow::response GroupController::GetGroups(const std::string& tournamentId){
    if (auto groups = this->groupDelegate->GetGroups(tournamentId)) {
        const nlohmann::json body = *groups;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::INTERNAL_SERVER_ERROR};
}
crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId){
    if (auto group = this->groupDelegate->GetGroup(tournamentId, groupId)) {
        const nlohmann::json body = *group;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::INTERNAL_SERVER_ERROR};
}
crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId){
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Group group = requestBody;

    auto groupId = groupDelegate->CreateGroup(tournamentId, group);
    crow::response response;
    if (groupId) {
        response.add_header("location", *groupId);
        response.code = crow::CREATED;
    } else {
        response.code = 422;
    }

    return response;
}

crow::response GroupController::UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId){
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Group group;
    group.Id() = groupId;
    group.Name() = requestBody["name"].get<std::string>();

    const auto result = groupDelegate->UpdateGroup(tournamentId, group);
    if (result) {
        return crow::response{crow::NO_CONTENT};
    }

    if (result.error() == "Group doesn't exist" || result.error() == "Tournament doesn't exist") {
        return crow::response{crow::NOT_FOUND};
    }

    return crow::response{422, result.error()};
}

crow::response GroupController::RemoveGroup(const std::string& tournamentId, const std::string& groupId) {
    const auto result = groupDelegate->RemoveGroup(tournamentId, groupId);
    if (result) {
        return crow::response{crow::NO_CONTENT};
    }

    if (result.error() == "Group doesn't exist" || result.error() == "Tournament doesn't exist") {
        return crow::response{crow::NOT_FOUND};
    }

    return crow::response{crow::INTERNAL_SERVER_ERROR};
}

crow::response GroupController::UpdateTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId) {
    const std::vector<domain::Team> teams = nlohmann::json::parse(request.body);
    const auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
    if (result) {
        return crow::response{crow::NO_CONTENT};
    }

    return crow::response{422, result.error()};
}
REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)
REGISTER_ROUTE(GroupController, RemoveGroup, "/tournaments/<string>/groups/<string>", "DELETE"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)
