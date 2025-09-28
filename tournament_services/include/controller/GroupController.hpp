#ifndef A7B3517D_1DC1_4B59_A78C_D3E03D29710C
#define A7B3517D_1DC1_4B59_A78C_D3E03D29710C

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
};

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate) : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController()
{
}

crow::response GroupController::GetGroups(const std::string& tournamentId){
    return crow::response{crow::NOT_IMPLEMENTED};
}
crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId){
    return crow::response{crow::NOT_IMPLEMENTED};
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
crow::response GroupController::UpdateGroup(const crow::request& request){
    return crow::response{crow::NOT_IMPLEMENTED};
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
