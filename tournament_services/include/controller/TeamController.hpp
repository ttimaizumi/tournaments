//
// Created by developer on 8/22/25.
//

#ifndef RESTAPI_TEAM_CONTROLLER_HPP
#define RESTAPI_TEAM_CONTROLLER_HPP

#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <regex>

#include "delegate/ITeamDelegate.hpp"
#include "domain/Constants.hpp"

class TeamController {
    std::shared_ptr<ITeamDelegate> teamDelegate;
public:
    explicit TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate);

    [[nodiscard]] crow::response getTeam(const std::string& teamId) const;
    [[nodiscard]] crow::response getAllTeams() const;
    [[nodiscard]] crow::response createTeam(const crow::request& request) const;
    [[nodiscard]] crow::response updateTeam(const crow::request& request, const std::string& teamId) const;
    [[nodiscard]] crow::response deleteTeam(const std::string& teamId) const;
};



#endif //RESTAPI_TEAM_CONTROLLER_HPP