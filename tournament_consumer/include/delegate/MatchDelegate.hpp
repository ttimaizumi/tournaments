//
// Created by developer on 10/13/25.
//

#ifndef CONSUMER_MATCHDELEGATE_HPP
#define CONSUMER_MATCHDELEGATE_HPP

#include <memory>

#include "event/TeamAddEvent.hpp"
#include "persistence/repository/IMatchRepository.hpp"

class MatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
public:
    MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository);
    void ProcessTeamAddition(const domain::TeamAddEvent& teamAddEvent);
};

inline MatchDelegate::MatchDelegate(const std::shared_ptr<IMatchRepository> &matchRepository) : matchRepository(matchRepository) {}

inline void MatchDelegate::ProcessTeamAddition(const domain::TeamAddEvent& teamAddEvent) {
    //If tournament has all teams and all matches are created?
    //then create matches
    //else move on
}

#endif //CONSUMER_MATCHDELEGATE_HPP