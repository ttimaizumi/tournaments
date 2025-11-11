//
// Created by edgar on 11/10/25.
//

#ifndef TOURNAMENTS_MATCHREPOSITORY_HPP
#define TOURNAMENTS_MATCHREPOSITORY_HPP
#pragma once

#include "IMatchRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"   // igual que TournamentRepository
#include <memory>

class MatchRepository : public IMatchRepository
{
    std::shared_ptr<PostgresConnectionProvider> connectionProvider;

public:
    explicit MatchRepository(std::shared_ptr<PostgresConnectionProvider> provider);

    std::vector<nlohmann::json>
    FindByTournament(std::string_view tournamentId,
                     std::optional<std::string> statusFilter) override;

    std::optional<nlohmann::json>
    FindByTournamentAndId(std::string_view tournamentId,
                          std::string_view matchId) override;

    bool
    UpdateScore(std::string_view tournamentId,
                std::string_view matchId,
                const nlohmann::json& newScore,
                std::string newStatus) override;
};

#endif //TOURNAMENTS_MATCHREPOSITORY_HPP