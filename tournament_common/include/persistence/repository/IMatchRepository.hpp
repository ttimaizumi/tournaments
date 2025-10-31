//
// Created by developer on 10/14/25.
//

#ifndef TOURNAMENTS_IMATCHREPOSITORY_HPP
#define TOURNAMENTS_IMATCHREPOSITORY_HPP

#include <string_view>
#include <vector>
#include <memory>

#include "domain/Match.hpp"

class IMatchRepository : public IRepository<domain::Match, std::string> {
public:
    virtual ~IMatchRepository() = default;
    //Find match with only one team to be added
    virtual domain::Match FindLastOpenMatch(const std::string_view& tournamentId) = 0;
    virtual std::vector<std::shared_ptr<domain::Match>> FindMatchesByTournamentAndRound(const std::string_view& tournamentId) = 0;
};
#endif //TOURNAMENTS_IMATCHREPOSITORY_HPP