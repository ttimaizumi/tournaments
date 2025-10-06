//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTCONTROLLER_HPP
#define TOURNAMENTS_TOURNAMENTCONTROLLER_HPP

#include <regex>
#include <memory>
#include <crow.h>

#include "delegate/ITournamentDelegate.hpp"


static const std::regex ID_VALUE_TOURNAMENT("[A-Za-z0-9\\-]+");

class TournamentController {
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;
public:
    explicit TournamentController(std::shared_ptr<ITournamentDelegate> tournament);

    [[nodiscard]] crow::response getTournament(const std::string& tournamentId) const;
    [[nodiscard]] crow::response updateTournament(const crow::request& request, const std::string& tournamentId) const;

    [[nodiscard]] crow::response CreateTournament(const crow::request &request) const;
    [[nodiscard]] crow::response ReadAll() const;
};


#endif //TOURNAMENTS_TOURNAMENTCONTROLLER_HPP