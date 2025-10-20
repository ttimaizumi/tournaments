#ifndef SUPPORT_STANDING_HPP
#define SUPPORT_STANDING_HPP

#include <string>

// Stub minimal para tests - Standing/Clasificación
struct Standing {
    std::string teamId;
    std::string tournamentId;
    std::string groupId;
    int points;
    int wins;
    int draws;
    int losses;
    int goalsFor;
    int goalsAgainst;
    int goalDifference;
    int matchesPlayed;

    Standing() : points(0), wins(0), draws(0), losses(0),
                 goalsFor(0), goalsAgainst(0), goalDifference(0), matchesPlayed(0) {}

    Standing(std::string tid, std::string trnId, std::string gid, int pts, int w, int d, int l,
             int gf, int ga, int gd, int mp)
        : teamId(tid), tournamentId(trnId), groupId(gid), points(pts), wins(w), draws(d),
          losses(l), goalsFor(gf), goalsAgainst(ga), goalDifference(gd), matchesPlayed(mp) {}
};

#endif // SUPPORT_STANDING_HPP
