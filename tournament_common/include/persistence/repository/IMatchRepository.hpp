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

class IMatchRepository {
public:
    virtual ~IMatchRepository() = default;

    // Lista de partidos por torneo (opcionalmente filtrado por status en el JSON: scheduled/played)
    virtual std::vector<nlohmann::json>
    FindByTournament(std::string_view tournamentId,
                     std::optional<std::string> statusFilter) = 0;

    // Un partido por torneo + matchId
    virtual std::optional<nlohmann::json>
    FindByTournamentAndId(std::string_view tournamentId,
                          std::string_view matchId) = 0;

    // Crear un match: regresa el id (uuid) si se inserto
    virtual std::optional<std::string>
    Create(const nlohmann::json& matchDocument) = 0;

    // Actualizar marcador y status (ej: played)
    virtual bool
    UpdateScore(std::string_view tournamentId,
                std::string_view matchId,
                const nlohmann::json& newScore,
                std::string newStatus) = 0;

    // Asignar participantes (slot home y/o visitor) en un match existente
    virtual bool
    UpdateParticipants(std::string_view tournamentId,
                       std::string_view matchId,
                       std::optional<std::string> homeTeamId,
                       std::optional<std::string> visitorTeamId) = 0;
};

#endif //TOURNAMENTS_IMATCHREPOSITORY_HPP
