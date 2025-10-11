// TournamentController.hpp
//
// Limpio: sin metodos duplicados

#ifndef TOURNAMENTS_TOURNAMENTCONTROLLER_HPP
#define TOURNAMENTS_TOURNAMENTCONTROLLER_HPP

#include <memory>
#include <string>
#include <crow.h>

#include "delegate/ITournamentDelegate.hpp"

class TournamentController {
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;

public:
    explicit TournamentController(std::shared_ptr<ITournamentDelegate> delegate);

    [[nodiscard]] crow::response CreateTournament(const crow::request& request) const;
    [[nodiscard]] crow::response ReadAll() const;
    [[nodiscard]] crow::response ReadById(const std::string& id) const;
    [[nodiscard]] crow::response UpdateTournament(const crow::request& request, const std::string& id) const;
};

#endif // TOURNAMENTS_TOURNAMENTCONTROLLER_HPP
