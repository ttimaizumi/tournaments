#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/GroupController.hpp"

#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"
#include "exception/Error.hpp"
#include <iostream>

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate) : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController()
{
}

static int mapErrorToStatus(const Error err) {
  switch (err) {
    case Error::NOT_FOUND: return crow::NOT_FOUND;
    case Error::INVALID_FORMAT: return crow::BAD_REQUEST;
    case Error::DUPLICATE: return crow::CONFLICT;
    case Error::UNPROCESSABLE_ENTITY: return crow::NOT_ACCEPTABLE;
    default: return crow::INTERNAL_SERVER_ERROR;
  }
}

crow::response GroupController::GetGroups(const std::string& tournamentId){
    auto groups = this->groupDelegate->GetGroups(tournamentId);
    if (groups) {
        const nlohmann::json body = *groups;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{ mapErrorToStatus(groups.error()), "Error" };
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId){
    auto result = this->groupDelegate->GetGroup(tournamentId, groupId);
    if (result) {
        const nlohmann::json body = *result;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{ mapErrorToStatus(result.error()), "Error" };
}

crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId){
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Group group = requestBody;

    auto groupId = groupDelegate->CreateGroup(tournamentId, group);
    if (groupId) {
        crow::response response;
        response.add_header("location", *groupId);
        response.code = crow::CREATED;
        return response;
    }
    return crow::response{ mapErrorToStatus(groupId.error()), "Error" };
}

crow::response GroupController::UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId) {
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Group group = requestBody;

    auto result = groupDelegate->UpdateGroup(tournamentId, group, groupId);
    if (result) {
        crow::response response{crow::NO_CONTENT};
        return response;
    }
    return crow::response{ mapErrorToStatus(result.error()), "Error" };
}

crow::response GroupController::AddTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId) {
    auto requestBody = nlohmann::json::parse(request.body);
    std::vector<domain::Team> teams = requestBody.get<std::vector<domain::Team>>();

    const auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
    if (result) {
        crow::response response{crow::NO_CONTENT};
        return response;
    }
    return crow::response{ mapErrorToStatus(result.error()), "Error" };
}

crow::response GroupController::RemoveGroup(const std::string& tournamentId, const std::string& groupId) {
    auto result = this->groupDelegate->RemoveGroup(tournamentId, groupId);
    if (result) {
        crow::response response{crow::NO_CONTENT};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{ mapErrorToStatus(result.error()), "Error" };
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method) 
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)
REGISTER_ROUTE(GroupController, AddTeams, "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)
REGISTER_ROUTE(GroupController, RemoveGroup, "/tournaments/<string>/groups/<string>", "DELETE"_method)
