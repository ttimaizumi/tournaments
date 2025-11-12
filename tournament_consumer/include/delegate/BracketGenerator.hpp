//
// Created by developer on 11/11/25.
//

#ifndef TOURNAMENTS_BRACKETGENERATOR_HPP
#define TOURNAMENTS_BRACKETGENERATOR_HPP

#include <vector>
#include <string>

#include "domain/Match.hpp"
#include "domain/Team.hpp"

class BracketGenerator {
public:
    // Generate 63 matches for 32-team double elimination
    // Returns matches with names: W0-W30 (winners), L0-L29 (losers), F0-F1 (finals)
    std::vector<domain::Match> GenerateMatches(
        const std::string& tournamentId, 
        const std::vector<domain::Team>& teams
    );

private:
    void GenerateWinnersBracket(
        std::vector<domain::Match>& matches,
        const std::string& tournamentId,
        const std::vector<domain::Team>& teams
    );

    void GenerateLosersBracket(
        std::vector<domain::Match>& matches,
        const std::string& tournamentId
    );

    void GenerateFinals(
        std::vector<domain::Match>& matches,
        const std::string& tournamentId
    );
};

#endif //TOURNAMENTS_BRACKETGENERATOR_HPP
