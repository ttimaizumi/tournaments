//
// Created by developer on 10/13/25.
//

#ifndef CONSUMER_MATCHDELEGATE_HPP
#define CONSUMER_MATCHDELEGATE_HPP

#include <memory>
#include <print>
#include <vector>
#include <algorithm>

#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "domain/Match.hpp"

class MatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
    std::shared_ptr<GroupRepository> groupRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;

    struct TeamStanding {
        std::string teamId;
        std::string teamName;
        int points = 0;
        int goalDifference = 0;
    };

    void CreateRegularMatchesForGroup(const std::string& tournamentId, const std::string& groupId, const std::vector<domain::Team>& teams);
    void CreatePlayoffMatches(const std::string& tournamentId);
    std::vector<TeamStanding> CalculateGroupStandings(const std::string& tournamentId, const std::string& groupId);
    bool AllRegularMatchesPlayed(const std::string& tournamentId);

public:
    MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository,
                  const std::shared_ptr<GroupRepository>& groupRepository,
                  const std::shared_ptr<TournamentRepository>& tournamentRepository);

    void ProcessTeamAddition(const domain::TeamAddEvent& teamAddEvent);
    void ProcessScoreUpdate(const domain::ScoreUpdateEvent& scoreUpdateEvent);
};

#endif //CONSUMER_MATCHDELEGATE_HPP