#ifndef SUPPORT_MATCH_HPP
#define SUPPORT_MATCH_HPP

#include <string>

// Stub minimal para tests - Match del Mundial
struct Match {
    std::string id;
    std::string tournamentId;
    std::string groupId;
    std::string homeTeamId;
    std::string awayTeamId;
    int homeScore;
    int awayScore;
    std::string phase;  // "GROUP" o "ELIMINATION"
    std::string round;  // "ROUND_1", "ROUND_2", "ROUND_3", "ROUND_OF_16", "QUARTER", "SEMI", "FINAL"
    bool isFinished;

    Match() : homeScore(0), awayScore(0), isFinished(false) {}

    Match(std::string id, std::string tid, std::string gid, std::string home, std::string away)
        : id(id), tournamentId(tid), groupId(gid), homeTeamId(home), awayTeamId(away),
          homeScore(0), awayScore(0), phase("GROUP"), isFinished(false) {}

    Match(std::string id, std::string tid, std::string gid, std::string home, std::string away,
          int hScore, int aScore, std::string ph, std::string rnd, bool fin)
        : id(id), tournamentId(tid), groupId(gid), homeTeamId(home), awayTeamId(away),
          homeScore(hScore), awayScore(aScore), phase(ph), round(rnd), isFinished(fin) {}
};

#endif // SUPPORT_MATCH_HPP
