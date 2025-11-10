//
// Created for tournament match system
//

#ifndef TOURNAMENTS_SCOREUPDATEEVENT_HPP
#define TOURNAMENTS_SCOREUPDATEEVENT_HPP

#include <string>
#include <nlohmann/json.hpp>
#include "domain/Match.hpp"

namespace domain {
    struct ScoreUpdateEvent {
        std::string tournamentId;
        std::string matchId;
        Score score;
    };

    inline void from_json(const nlohmann::json &json, ScoreUpdateEvent &event) {
        json.at("tournamentId").get_to(event.tournamentId);
        json.at("matchId").get_to(event.matchId);
        json.at("score").get_to(event.score);
    }
}

#endif //TOURNAMENTS_SCOREUPDATEEVENT_HPP
