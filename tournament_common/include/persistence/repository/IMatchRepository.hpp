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

    virtual std::vector<std::shared_ptr<domain::Match>> FindByTournamentId(const std::string_view& tournamentId) = 0;
    virtual std::vector<std::shared_ptr<domain::Match>> FindPlayedByTournamentId(const std::string_view& tournamentId) = 0;
    virtual std::vector<std::shared_ptr<domain::Match>> FindPendingByTournamentId(const std::string_view& tournamentId) = 0;
    virtual std::shared_ptr<domain::Match> FindByTournamentIdAndMatchId(const std::string_view& tournamentId, const std::string_view& matchId) = 0;
    virtual std::shared_ptr<domain::Match> FindLastOpenMatch(const std::string_view& tournamentId) = 0;
    virtual std::vector<std::shared_ptr<domain::Match>> FindMatchesByTournamentAndRound(const std::string_view& tournamentId, domain::Round round) = 0;
    virtual bool TournamentExists(const std::string_view& tournamentId) = 0;
};
#endif //TOURNAMENTS_IMATCHREPOSITORY_HPP