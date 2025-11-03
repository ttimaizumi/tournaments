#ifndef A7B3517D_1DC1_4B59_A78C_D3E03D29710C
#define A7B3517D_1DC1_4B59_A78C_D3E03D29710C

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <vector>
#include <string>
#include <memory>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Utilities.hpp"


class GroupController
{
    std::shared_ptr<IGroupDelegate> groupDelegate;
public:
    GroupController(const std::shared_ptr<IGroupDelegate>& delegate);
    ~GroupController();
    crow::response GetGroups(const std::string& tournamentId);
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId);
    crow::response CreateGroup(const crow::request& request, const std::string& tournamentId);
    crow::response UpdateGroup(const crow::request& request);
    crow::response AddTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
    crow::response GetHealth();
};

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
        groupId.error();
        response.code = 422;
    }

    return response;
}

crow::response GroupController::UpdateGroup(const crow::request& request){
    return crow::response{crow::NOT_IMPLEMENTED};
}

crow::response GroupController::AddTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId) {
    const std::vector<domain::Team> teams = nlohmann::json::parse(request.body);
    const auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
    if (result) {
        return crow::response{crow::NO_CONTENT};
    }

    return crow::response{422, result.error()};
}

crow::response GroupController::GetHealth(){
    return crow::response{crow::OK};
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)
REGISTER_ROUTE(GroupController, AddTeams, "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)
REGISTER_ROUTE(GroupController, GetHealth, "/health", "GET"_method)

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
