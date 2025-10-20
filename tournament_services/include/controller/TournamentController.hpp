//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTCONTROLLER_HPP
#define TOURNAMENTS_TOURNAMENTCONTROLLER_HPP

#include <regex>
#include <memory>
#include <crow.h>

#include "delegate/ITournamentDelegate.hpp"
#include "domain/Constants.hpp"

class TournamentController {
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;
public:
    TournamentController(const std::shared_ptr<ITournamentDelegate>& delegate);
    ~TournamentController();

    crow::response getTournament(const std::string& tournamentId);
    crow::response updateTournament(const crow::request& request, const std::string& tournamentId);
    crow::response CreateTournament(const crow::request& request);
    crow::response ReadAll();
    crow::response deleteTournament(const std::string& tournamentId);
};

#endif //TOURNAMENTS_TOURNAMENTCONTROLLER_HPP