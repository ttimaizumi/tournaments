//
// Created by developer on 11/11/25.
//

#include "delegate/BracketGenerator.hpp"
#include <stdexcept>

std::vector<domain::Match> BracketGenerator::GenerateMatches(
    const std::string& tournamentId,
    const std::vector<domain::Team>& teams
) {
    if (teams.size() != 32) {
        throw std::invalid_argument("Double elimination strategy requires exactly 32 teams");
    }

    std::vector<domain::Match> matches;
    
    // Generate all three bracket sections
    GenerateWinnersBracket(matches, tournamentId, teams);
    GenerateLosersBracket(matches, tournamentId);
    GenerateFinals(matches, tournamentId);
    
    return matches;
}

void BracketGenerator::GenerateWinnersBracket(
    std::vector<domain::Match>& matches,
    const std::string& tournamentId,
    const std::vector<domain::Team>& teams
) {
    // Round 1: 16 matches (W0-W15) with teams assigned
    for (int i = 0; i < 16; ++i) {
        domain::Match match;
        match.Name() = "W" + std::to_string(i);
        match.TournamentId() = tournamentId;
        match.HomeTeamId() = teams[i * 2].Id;
        match.VisitorTeamId() = teams[i * 2 + 1].Id;
        matches.push_back(match);
    }
    
    // Round 2: 8 matches (W16-W23) 
    for (int i = 16; i < 24; ++i) {
        domain::Match match;
        match.Name() = "W" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Round 3: 4 matches (W24-W27)
    for (int i = 24; i < 28; ++i) {
        domain::Match match;
        match.Name() = "W" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Round 4: 2 matches (W28-W29)
    for (int i = 28; i < 30; ++i) {
        domain::Match match;
        match.Name() = "W" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Round 5: 1 match (W30) - Winners Bracket Finals
    domain::Match wbFinal;
    wbFinal.Name() = "W30";
    wbFinal.TournamentId() = tournamentId;
    matches.push_back(wbFinal);
}

void BracketGenerator::GenerateLosersBracket(
    std::vector<domain::Match>& matches,
    const std::string& tournamentId
) {
    // Losers Round 1: 8 matches (L0-L7)
    for (int i = 0; i < 8; ++i) {
        domain::Match match;
        match.Name() = "L" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Losers Round 2: 8 matches (L8-L15)
    for (int i = 8; i < 16; ++i) {
        domain::Match match;
        match.Name() = "L" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Losers Round 3: 4 matches (L16-L19)
    for (int i = 16; i < 20; ++i) {
        domain::Match match;
        match.Name() = "L" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Losers Round 4: 4 matches (L20-L23)
    for (int i = 20; i < 24; ++i) {
        domain::Match match;
        match.Name() = "L" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Losers Round 5: 2 matches (L24-L25)
    for (int i = 24; i < 26; ++i) {
        domain::Match match;
        match.Name() = "L" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Losers Round 6: 2 matches (L26-L27)
    for (int i = 26; i < 28; ++i) {
        domain::Match match;
        match.Name() = "L" + std::to_string(i);
        match.TournamentId() = tournamentId;
        matches.push_back(match);
    }
    
    // Losers Round 7: 1 match (L28)
    domain::Match lbSemiFinal;
    lbSemiFinal.Name() = "L28";
    lbSemiFinal.TournamentId() = tournamentId;
    matches.push_back(lbSemiFinal);
    
    // Losers Round 8: 1 match (L29) - Losers Bracket Final
    domain::Match lbFinal;
    lbFinal.Name() = "L29";
    lbFinal.TournamentId() = tournamentId;
    matches.push_back(lbFinal);
}

void BracketGenerator::GenerateFinals(
    std::vector<domain::Match>& matches,
    const std::string& tournamentId
) {
    // Final 1 (F0)
    domain::Match grandFinal1;
    grandFinal1.Name() = "F0";
    grandFinal1.TournamentId() = tournamentId;
    matches.push_back(grandFinal1);
    
    // Final 2 (F1) - Bracket Reset
    domain::Match grandFinal2;
    grandFinal2.Name() = "F1";
    grandFinal2.TournamentId() = tournamentId;
    matches.push_back(grandFinal2);
}
