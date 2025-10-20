//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_ITOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_ITOURNAMENTDELEGATE_HPP

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "domain/Tournament.hpp"
#include "exception/Error.hpp"

class ITournamentDelegate {
public:
    virtual ~ITournamentDelegate() = default;

    virtual std::expected<std::vector<std::shared_ptr<domain::Tournament>>, Error> ReadAll() = 0;

    virtual std::expected<std::shared_ptr<domain::Tournament>, Error> GetTournament(std::string_view id) = 0;
    virtual std::expected<std::string, Error> CreateTournament(const domain::Tournament& tournament) = 0;
    virtual std::expected<std::string, Error> UpdateTournament(const domain::Tournament& tournament) = 0;
    virtual std::expected<void, Error> DeleteTournament(std::string_view id) = 0;
};


#endif //TOURNAMENTS_ITOURNAMENTDELEGATE_HPP