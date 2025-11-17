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
    crow::response UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
    crow::response AddTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
    crow::response RemoveGroup(const std::string& tournamentId, const std::string& groupId);
};

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
