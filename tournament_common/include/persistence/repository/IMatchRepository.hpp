//
// Created by developer on 10/14/25.
//

#ifndef TOURNAMENTS_IMATCHREPOSITORY_HPP
#define TOURNAMENTS_IMATCHREPOSITORY_HPP

#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

class IMatchRepository
{
public:
    virtual ~IMatchRepository() = default;

    // Regresa todos los partidos del torneo, con filtro opcional de status (played/pending)
    virtual std::vector<nlohmann::json>
    FindByTournament(std::string_view tournamentId,
                     std::optional<std::string> statusFilter) = 0;

    // Regresa un solo partido por torneo + matchId
    virtual std::optional<nlohmann::json>
    FindByTournamentAndId(std::string_view tournamentId,
                          std::string_view matchId) = 0;

    // Actualizar marcador y status (ej: played)
    virtual bool
    UpdateScore(std::string_view tournamentId,
                std::string_view matchId,
                const nlohmann::json& newScore,
                std::string newStatus) = 0;
};

#endif //TOURNAMENTS_IMATCHREPOSITORY_HPP