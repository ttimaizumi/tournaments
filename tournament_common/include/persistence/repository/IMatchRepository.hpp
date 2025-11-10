//
// Created by developer on 10/14/25.
//

#ifndef TOURNAMENTS_IMATCHREPOSITORY_HPP
#define TOURNAMENTS_IMATCHREPOSITORY_HPP

#include <string_view>
#include <vector>
#include <memory>

#include "domain/Match.hpp"
#include "IRepository.hpp"

class IMatchRepository : public IRepository<domain::Match, std::string> {
public:
    virtual ~IMatchRepository() = default;

    // Find all matches for a tournament
    virtual std::vector<std::shared_ptr<domain::Match>> FindByTournamentId(const std::string_view& tournamentId) = 0;

    // Find matches with score (played)
    virtual std::vector<std::shared_ptr<domain::Match>> FindPlayedByTournamentId(const std::string_view& tournamentId) = 0;

    // Find matches without score (pending)
    virtual std::vector<std::shared_ptr<domain::Match>> FindPendingByTournamentId(const std::string_view& tournamentId) = 0;

    // Find match by tournament and match ID
    virtual std::shared_ptr<domain::Match> FindByTournamentIdAndMatchId(const std::string_view& tournamentId, const std::string_view& matchId) = 0;

    // Find match with only one team to be added
    virtual std::shared_ptr<domain::Match> FindLastOpenMatch(const std::string_view& tournamentId) = 0;

    // Find matches by tournament and round
    virtual std::vector<std::shared_ptr<domain::Match>> FindMatchesByTournamentAndRound(const std::string_view& tournamentId, domain::Round round) = 0;

    // Check if tournament exists
    virtual bool TournamentExists(const std::string_view& tournamentId) = 0;
};
#endif //TOURNAMENTS_IMATCHREPOSITORY_HPP