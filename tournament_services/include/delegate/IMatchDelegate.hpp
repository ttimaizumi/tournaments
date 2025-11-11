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

class IMatchDelegate
{
public:
    virtual ~IMatchDelegate() = default;

    // Lista de partidos (JSON como en el doc)
    virtual std::expected<std::vector<nlohmann::json>, std::string>
    GetMatches(const std::string_view& tournamentId,
               std::optional<std::string> showMatches) = 0;

    // Un partido
    virtual std::expected<nlohmann::json, std::string>
    GetMatch(const std::string_view& tournamentId,
             const std::string_view& matchId) = 0;

    // Actualizar marcador
    virtual std::expected<void, std::string>
    UpdateScore(const std::string_view& tournamentId,
                const std::string_view& matchId,
                int homeScore,
                int visitorScore) = 0;
};

#endif //TOURNAMENTS_IMATCHDELEGATE_HPP