//
// Created by edgar on 11/10/25.
//
#ifndef TOURNAMENTS_MATCHREPOSITORY_HPP
#define TOURNAMENTS_MATCHREPOSITORY_HPP
#pragma once

#include "IMatchRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include <memory>

class MatchRepository : public IMatchRepository {
    std::shared_ptr<IDbConnectionProvider> dbProvider;

public:
    explicit MatchRepository(std::shared_ptr<IDbConnectionProvider> provider)
        : dbProvider(std::move(provider)) {}

    std::vector<nlohmann::json>
    FindByTournament(std::string_view tournamentId,
                     std::optional<std::string> statusFilter) override;

    std::optional<nlohmann::json>
    FindByTournamentAndId(std::string_view tournamentId,
                          std::string_view matchId) override;

    std::optional<std::string>
    Create(const nlohmann::json& matchDocument) override;

    bool
    UpdateScore(std::string_view tournamentId,
                std::string_view matchId,
                const nlohmann::json& newScore,
                std::string newStatus) override;

    bool
    UpdateParticipants(std::string_view tournamentId,
                       std::string_view matchId,
                       std::optional<std::string> homeTeamId,
                       std::optional<std::string> visitorTeamId) override;
};

#endif //TOURNAMENTS_MATCHREPOSITORY_HPP
