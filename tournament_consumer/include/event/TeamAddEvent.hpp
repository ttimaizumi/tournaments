//
// Created by developer on 10/14/25.
//

#ifndef TOURNAMENTS_GROUPADDEVENT_HPP
#define TOURNAMENTS_GROUPADDEVENT_HPP
#include <string>

struct TeamAddEvent {
    std::string tournamentId;
    std::string groupId;
    std::string teamId;
};
#endif //TOURNAMENTS_GROUPADDEVENT_HPP