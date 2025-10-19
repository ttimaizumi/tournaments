// tournament_services/include/controller/GroupController.hpp
#ifndef A7B3517D_1DC1_4B59_A78C_D3E03D29710C
#define A7B3517D_1DC1_4B59_A78C_D3E03D29710C

#include <memory>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"

class GroupController {
    std::shared_ptr<IGroupDelegate> groupDelegate;

public:
    explicit GroupController(const std::shared_ptr<IGroupDelegate>& delegate);
    ~GroupController();

    crow::response GetGroups(const std::string& tournamentId);
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId);
    crow::response CreateGroup(const crow::request& request, const std::string& tournamentId);
    crow::response UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
    crow::response UpdateTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
};

#endif
