//
// Created by edgar on 11/10/25.
//
#ifndef TOURNAMENTS_IMATCHDELEGATE_HPP
#define TOURNAMENTS_IMATCHDELEGATE_HPP
#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;

    // Crear match con arbol fijo (advancement opcional)
    // Body esperado:
    // {
    //   "homeTeamId": "X" | null,
    //   "visitorTeamId": "Y" | null,
    //   "bracket": "winners" | "losers",
    //   "round": 1,
    //   "advancement": {
    //     "winner": {"matchId":"...", "slot":"home|visitor"},
    //     "loser":  {"matchId":"...", "slot":"home|visitor"}
    //   }
    // }
    virtual std::expected<nlohmann::json, std::string>
    CreateMatch(const std::string_view& tournamentId,
                const nlohmann::json& body) = 0;

    // Lista y detalle
    virtual std::expected<std::vector<nlohmann::json>, std::string>
    GetMatches(const std::string_view& tournamentId,
               std::optional<std::string> statusFilter) = 0;

    virtual std::expected<nlohmann::json, std::string>
    GetMatch(const std::string_view& tournamentId,
             const std::string_view& matchId) = 0;

    // Actualizar marcador (no empates)
    virtual std::expected<void, std::string>
    UpdateScore(const std::string_view& tournamentId,
                const std::string_view& matchId,
                int homeScore,
                int visitorScore) = 0;
};

#endif //TOURNAMENTS_IMATCHDELEGATE_HPP
