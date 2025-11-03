//
// Created by developer on 10/14/25.
//

#ifndef TOURNAMENTS_GROUPADDEVENT_HPP
#define TOURNAMENTS_GROUPADDEVENT_HPP
#include <string>
#include <nlohmann/json.hpp>

namespace domain {
    struct TeamAddEvent {
        std::string tournamentId;
        std::string groupId;
        std::string teamId;
    };

    inline void from_json(const nlohmann::json &json, TeamAddEvent &teamAddEvent) {
        json.at("tournamentId").get_to(teamAddEvent.tournamentId);
        json.at("groupId").get_to(teamAddEvent.groupId);
        json.at("teamId").get_to(teamAddEvent.teamId);
    }
}
#endif //TOURNAMENTS_GROUPADDEVENT_HPP
