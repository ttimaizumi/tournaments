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
    void ProcessTeamAddition(const TeamAddEvent& teamAddEvent);
};

inline MatchDelegate::MatchDelegate(const std::shared_ptr<IMatchRepository> &matchRepository) : matchRepository(matchRepository) {}

inline void MatchDelegate::ProcessTeamAddition(const TeamAddEvent& teamAddEvent) {
    //If there's a match to assign the team?
    //Add team to match
    //If no open match is found the creation a match with team


}

#endif //CONSUMER_MATCHDELEGATE_HPP