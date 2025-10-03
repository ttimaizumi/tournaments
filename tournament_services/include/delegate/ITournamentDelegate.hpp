//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_ITOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_ITOURNAMENTDELEGATE_HPP

#include <string>
#include <memory>

#include "domain/Tournament.hpp"

class ITournamentDelegate {
public:
    virtual ~ITournamentDelegate() = default;
    virtual std::shared_ptr<domain::Tournament> GetTournament(std::string_view id) = 0;

    virtual std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) = 0;
    virtual std::vector<std::shared_ptr<domain::Tournament>> ReadAll() = 0;

    virtual std::string UpdateTournament(const domain::Tournament& tournament) = 0;
};

#endif //TOURNAMENTS_ITOURNAMENTDELEGATE_HPP