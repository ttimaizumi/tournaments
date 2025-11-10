#include "delegate/MatchDelegate.hpp"
#include "domain/Utilities.hpp"
#include <iostream>
#include <map>

MatchDelegate::MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository,
                             const std::shared_ptr<GroupRepository>& groupRepository,
                             const std::shared_ptr<TournamentRepository>& tournamentRepository)
    : matchRepository(matchRepository),
      groupRepository(groupRepository),
      tournamentRepository(tournamentRepository) {}

void MatchDelegate::ProcessTeamAddition(const domain::TeamAddEvent& teamAddEvent) {
    try {
        auto group = groupRepository->FindByTournamentIdAndGroupId(teamAddEvent.tournamentId, teamAddEvent.groupId);

        if (group == nullptr) {
            std::println("Group not found: {}", teamAddEvent.groupId);
            return;
        }

        // Check if this group now has 4 teams (complete)
        if (group->Teams().size() == 4) {
            std::println("Group {} is complete with 4 teams. Checking if matches exist...", teamAddEvent.groupId);

            // Check if matches already exist for this group by checking if any regular matches
            // exist between teams in this group
            auto regularMatches = matchRepository->FindMatchesByTournamentAndRound(teamAddEvent.tournamentId, domain::Round::REGULAR);

            bool matchesExist = false;
            for (const auto& match : regularMatches) {
                // Check if this match involves teams from this group
                for (const auto& team : group->Teams()) {
                    if (match->HomeTeamId() == team.Id || match->VisitorTeamId() == team.Id) {
                        matchesExist = true;
                        break;
                    }
                }
                if (matchesExist) break;
            }

            if (matchesExist) {
                std::println("Matches already exist for group {}, skipping creation", teamAddEvent.groupId);
            } else {
                std::println("Creating regular matches for group {}...", teamAddEvent.groupId);
                CreateRegularMatchesForGroup(teamAddEvent.tournamentId, teamAddEvent.groupId, group->Teams());
            }
        } else {
            std::println("Group {} has {} teams, waiting for more...", teamAddEvent.groupId, group->Teams().size());
        }
    } catch (const std::exception& e) {
        std::println("Error processing team addition: {}", e.what());
    }
}

void MatchDelegate::CreateRegularMatchesForGroup(const std::string& tournamentId, const std::string& groupId, const std::vector<domain::Team>& teams) {
    // Create round-robin matches for the group (6 matches for 4 teams)
    for (size_t i = 0; i < teams.size(); ++i) {
        for (size_t j = i + 1; j < teams.size(); ++j) {
            domain::Match match;
            match.SetTournamentId(tournamentId);
            match.SetHomeTeamId(teams[i].Id);
            match.SetHomeTeamName(teams[i].Name);
            match.SetVisitorTeamId(teams[j].Id);
            match.SetVisitorTeamName(teams[j].Name);
            match.SetRound(domain::Round::REGULAR);

            std::string matchId = matchRepository->Create(match);
            std::println("Created regular match: {} vs {} (ID: {})", teams[i].Name, teams[j].Name, matchId);
        }
    }
}

void MatchDelegate::ProcessScoreUpdate(const domain::ScoreUpdateEvent& scoreUpdateEvent) {
    try {
        std::println("Processing score update for match {} in tournament {}",
                     scoreUpdateEvent.matchId, scoreUpdateEvent.tournamentId);

        // Get the match that was updated
        auto updatedMatch = matchRepository->FindByTournamentIdAndMatchId(
            scoreUpdateEvent.tournamentId, scoreUpdateEvent.matchId);

        if (updatedMatch == nullptr) {
            std::println("Match not found: {}", scoreUpdateEvent.matchId);
            return;
        }

        // Check if this is a regular season match
        if (updatedMatch->GetRound() == domain::Round::REGULAR) {
            // Check if ALL regular matches are now complete
            if (AllRegularMatchesPlayed(scoreUpdateEvent.tournamentId)) {
                std::println("All regular matches complete! Creating playoff matches...");
                CreatePlayoffMatches(scoreUpdateEvent.tournamentId);
            } else {
                std::println("Waiting for more regular matches to complete...");
            }
        } else {
            // This is a playoff match
            domain::Winner winner = scoreUpdateEvent.score.GetWinner();
            std::string winnerTeamName = (winner == domain::Winner::HOME) ?
                updatedMatch->HomeTeamName() : updatedMatch->VisitorTeamName();

            std::println("Winner of {} match: {}", domain::roundToString(updatedMatch->GetRound()), winnerTeamName);

            // Check if this was the final
            if (updatedMatch->GetRound() == domain::Round::FINAL) {
                std::println("Tournament complete! Champion: {}", winnerTeamName);
                return;
            }

            // Check if all matches in this round are complete
            if (AllRoundMatchesPlayed(scoreUpdateEvent.tournamentId, updatedMatch->GetRound())) {
                std::println("All {} matches complete! Creating next round...",
                             domain::roundToString(updatedMatch->GetRound()));
                CreateNextRoundMatches(scoreUpdateEvent.tournamentId, updatedMatch->GetRound());
            } else {
                std::println("Team {} advances. Waiting for more {} matches to complete...",
                             winnerTeamName, domain::roundToString(updatedMatch->GetRound()));
            }
        }
    } catch (const std::exception& e) {
        std::println("Error processing score update: {}", e.what());
    }
}

bool MatchDelegate::AllRegularMatchesPlayed(const std::string& tournamentId) {
    auto regularMatches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, domain::Round::REGULAR);

    // A MUNDIAL tournament with 8 groups of 4 teams each has 48 regular matches (6 per group)
    const size_t EXPECTED_REGULAR_MATCHES = 48;

    if (regularMatches.size() < EXPECTED_REGULAR_MATCHES) {
        return false;
    }

    // Check if all matches have scores
    for (const auto& match : regularMatches) {
        if (!match->HasScore()) {
            return false;
        }
    }

    return true;
}

std::vector<MatchDelegate::TeamStanding> MatchDelegate::CalculateGroupStandings(const std::string& tournamentId, const std::string& groupId) {
    // Get the group
    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return {};
    }

    // Initialize standings
    std::map<std::string, TeamStanding> standings;
    for (const auto& team : group->Teams()) {
        TeamStanding standing;
        standing.teamId = team.Id;
        standing.teamName = team.Name;
        standings[team.Id] = standing;
    }

    // Get all regular matches for this group's teams
    auto regularMatches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, domain::Round::REGULAR);

    // Calculate points and goal difference
    for (const auto& match : regularMatches) {
        // Check if this match involves teams from this group
        if (standings.find(match->HomeTeamId()) == standings.end() ||
            standings.find(match->VisitorTeamId()) == standings.end()) {
            continue;
        }

        if (!match->HasScore()) {
            continue;
        }

        const auto& score = match->MatchScore().value();

        if (score.IsTie()) {
            // Draw: 1 point each
            standings[match->HomeTeamId()].points += 1;
            standings[match->VisitorTeamId()].points += 1;
        } else if (score.GetWinner() == domain::Winner::HOME) {
            // Home win: 3 points
            standings[match->HomeTeamId()].points += 3;
            standings[match->HomeTeamId()].goalDifference += score.homeTeamScore - score.visitorTeamScore;
            standings[match->VisitorTeamId()].goalDifference += score.visitorTeamScore - score.homeTeamScore;
        } else {
            // Visitor win: 3 points
            standings[match->VisitorTeamId()].points += 3;
            standings[match->VisitorTeamId()].goalDifference += score.visitorTeamScore - score.homeTeamScore;
            standings[match->HomeTeamId()].goalDifference += score.homeTeamScore - score.visitorTeamScore;
        }
    }

    // Convert map to vector and sort
    std::vector<TeamStanding> result;
    for (const auto& [teamId, standing] : standings) {
        result.push_back(standing);
    }

    // Sort by points (descending), then by goal difference (descending)
    std::sort(result.begin(), result.end(), [](const TeamStanding& a, const TeamStanding& b) {
        if (a.points != b.points) {
            return a.points > b.points;
        }
        return a.goalDifference > b.goalDifference;
    });

    return result;
}

void MatchDelegate::CreatePlayoffMatches(const std::string& tournamentId) {
    // Get all groups for this tournament
    auto groups = groupRepository->FindByTournamentId(tournamentId);

    if (groups.size() != 8) {
        std::println("Expected 8 groups for MUNDIAL tournament, found {}", groups.size());
        return;
    }

    // Calculate standings for each group and get top 2 teams
    std::vector<std::pair<std::string, std::string>> qualifiedTeams; // (id, name)

    for (const auto& group : groups) {
        auto standings = CalculateGroupStandings(tournamentId, group->Id());

        if (standings.size() >= 2) {
            qualifiedTeams.push_back({standings[0].teamId, standings[0].teamName});
            qualifiedTeams.push_back({standings[1].teamId, standings[1].teamName});
            std::println("Group {}: {} and {} qualified", group->Name(), standings[0].teamName, standings[1].teamName);
        }
    }

    if (qualifiedTeams.size() != 16) {
        std::println("Expected 16 qualified teams, found {}", qualifiedTeams.size());
        return;
    }

    // Create round of 16 (eighths) matches
    // Typical World Cup bracket: Group A 1st vs Group B 2nd, Group C 1st vs Group D 2nd, etc.
    std::vector<std::pair<int, int>> bracket = {
        {0, 3}, {1, 2},   // Group A1 vs B2, A2 vs B1
        {4, 7}, {5, 6},   // Group C1 vs D2, C2 vs D1
        {8, 11}, {9, 10}, // Group E1 vs F2, E2 vs F1
        {12, 15}, {13, 14} // Group G1 vs H2, G2 vs H1
    };

    for (const auto& [homeIdx, visitorIdx] : bracket) {
        domain::Match match;
        match.SetTournamentId(tournamentId);
        match.SetHomeTeamId(qualifiedTeams[homeIdx].first);
        match.SetHomeTeamName(qualifiedTeams[homeIdx].second);
        match.SetVisitorTeamId(qualifiedTeams[visitorIdx].first);
        match.SetVisitorTeamName(qualifiedTeams[visitorIdx].second);
        match.SetRound(domain::Round::EIGHTHS);

        std::string matchId = matchRepository->Create(match);
        std::println("Created eighths match: {} vs {} (ID: {})",
                     qualifiedTeams[homeIdx].second, qualifiedTeams[visitorIdx].second, matchId);
    }

    std::println("Playoff bracket created successfully with {} eighths matches", bracket.size());
}

bool MatchDelegate::AllRoundMatchesPlayed(const std::string& tournamentId, domain::Round round) {
    auto matches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, round);

    // Check if all matches have scores
    for (const auto& match : matches) {
        if (!match->HasScore()) {
            return false;
        }
    }

    return !matches.empty();
}

void MatchDelegate::CreateNextRoundMatches(const std::string& tournamentId, domain::Round currentRound) {
    // Get all completed matches from current round
    auto currentMatches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, currentRound);

    // Sort matches by ID to ensure consistent bracket pairing
    std::sort(currentMatches.begin(), currentMatches.end(),
              [](const auto& a, const auto& b) { return a->Id() < b->Id(); });

    // Determine next round
    domain::Round nextRound;
    size_t expectedMatches;

    switch (currentRound) {
        case domain::Round::EIGHTHS:
            nextRound = domain::Round::QUARTERS;
            expectedMatches = 8;
            break;
        case domain::Round::QUARTERS:
            nextRound = domain::Round::SEMIS;
            expectedMatches = 4;
            break;
        case domain::Round::SEMIS:
            nextRound = domain::Round::FINAL;
            expectedMatches = 2;
            break;
        default:
            std::println("Cannot create next round after {}", domain::roundToString(currentRound));
            return;
    }

    if (currentMatches.size() != expectedMatches) {
        std::println("Expected {} matches in {}, found {}",
                     expectedMatches, domain::roundToString(currentRound), currentMatches.size());
        return;
    }

    // Create matches by pairing winners: [0,1], [2,3], [4,5], [6,7]
    for (size_t i = 0; i < currentMatches.size(); i += 2) {
        auto match1 = currentMatches[i];
        auto match2 = currentMatches[i + 1];

        if (!match1->HasScore() || !match2->HasScore()) {
            continue;
        }

        // Get winners
        auto winner1 = match1->MatchScore().value().GetWinner();
        auto winner2 = match2->MatchScore().value().GetWinner();

        std::string homeTeamId = (winner1 == domain::Winner::HOME) ?
            match1->HomeTeamId() : match1->VisitorTeamId();
        std::string homeTeamName = (winner1 == domain::Winner::HOME) ?
            match1->HomeTeamName() : match1->VisitorTeamName();

        std::string visitorTeamId = (winner2 == domain::Winner::HOME) ?
            match2->HomeTeamId() : match2->VisitorTeamId();
        std::string visitorTeamName = (winner2 == domain::Winner::HOME) ?
            match2->HomeTeamName() : match2->VisitorTeamName();

        // Create next round match
        domain::Match newMatch;
        newMatch.SetTournamentId(tournamentId);
        newMatch.SetHomeTeamId(homeTeamId);
        newMatch.SetHomeTeamName(homeTeamName);
        newMatch.SetVisitorTeamId(visitorTeamId);
        newMatch.SetVisitorTeamName(visitorTeamName);
        newMatch.SetRound(nextRound);

        std::string matchId = matchRepository->Create(newMatch);
        std::println("Created {} match: {} vs {} (ID: {})",
                     domain::roundToString(nextRound), homeTeamName, visitorTeamName, matchId);
    }

    std::println("{} round created successfully", domain::roundToString(nextRound));
}
