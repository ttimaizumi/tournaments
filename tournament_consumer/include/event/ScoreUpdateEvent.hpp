//
// Created by developer on 11/12/25.
//

#ifndef TOURNAMENTS_SCOREUPDATEEVENT_HPP
#define TOURNAMENTS_SCOREUPDATEEVENT_HPP

#include <string>
#include <nlohmann/json.hpp>

namespace domain {
    struct ScoreUpdateEvent {
        std::string tournamentId;
        std::string matchId;
        int homeTeamScore;
        int visitorTeamScore;
    };

    inline void from_json(const nlohmann::json &json, ScoreUpdateEvent &event) {
        json.at("tournamentId").get_to(event.tournamentId);
        json.at("matchId").get_to(event.matchId);
        json.at("homeTeamScore").get_to(event.homeTeamScore);
        json.at("visitorTeamScore").get_to(event.visitorTeamScore);
    }
}

#endif //TOURNAMENTS_SCOREUPDATEEVENT_HPP
