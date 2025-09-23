//
// Created by developer on 8/22/25.
//

#ifndef RESTAPI_TEAM_CONTROLLER_HPP
#define RESTAPI_TEAM_CONTROLLER_HPP

#include <memory>
#include <crow.h>

#include "delegate/ITeamDelegate.hpp"

class TeamController {
    std::shared_ptr<ITeamDelegate> teamDelegate;
public:
    explicit TeamController(std::shared_ptr<ITeamDelegate> teamDelegate);

    [[nodiscard]] crow::response getTeam(const std::string& teamId) const;
    [[nodiscard]] crow::response getAllTeams() const;
    [[nodiscard]] crow::response CreateTeam(const crow::request& request) const;
};

#endif //RESTAPI_TEAM_CONTROLLER_HPP