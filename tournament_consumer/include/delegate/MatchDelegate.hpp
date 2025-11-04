//
// Created by developer on 10/13/25.
//

#ifndef CONSUMER_MATCHDELEGATE_HPP
#define CONSUMER_MATCHDELEGATE_HPP

#include <memory>
#include <print>

#include "event/TeamAddEvent.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/IMatchRepository.hpp"

class MatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
    std::shared_ptr<GroupRepository> groupRepository;
public:
    MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository, const std::shared_ptr<GroupRepository>& groupRepository);
    void ProcessTeamAddition(const domain::TeamAddEvent& teamAddEvent);
};

inline MatchDelegate::MatchDelegate(const std::shared_ptr<IMatchRepository> &matchRepository, const std::shared_ptr<GroupRepository> &groupRepository)
: matchRepository(matchRepository), groupRepository(groupRepository) {}

inline void MatchDelegate::ProcessTeamAddition(const domain::TeamAddEvent& teamAddEvent) {
    auto group = groupRepository->FindByTournamentIdAndGroupId(teamAddEvent.tournamentId, teamAddEvent.groupId);
    //If tournament has all teams and all matches are created?
    //then create matches
    if (group != nullptr && group->Teams().size() == 32) {
        std::println("creating matches for {}", teamAddEvent.tournamentId, group->Teams().size());
    }
    std::println("{} wait for teams, current teams: {}", teamAddEvent.tournamentId, group->Teams().size());
    std::cout << "This appears in docker logs" << std::endl;
    //else move on
}

#endif //CONSUMER_MATCHDELEGATE_HPP