//
// Created by tsuny on 9/1/25.
//

#include <memory>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

#include "persistence/repository/TournamentRepository.hpp"
#include "domain/Utilities.hpp"
#include "persistence/configuration/PostgresConnection.hpp"

TournamentRepository::TournamentRepository(std::shared_ptr<IDbConnectionProvider> connection)
    : connectionProvider(std::move(connection)) {}

std::shared_ptr<domain::Tournament> TournamentRepository::ReadById(const std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"select_tournament_by_id"}, pqxx::params{id});
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    nlohmann::json rowTournament = nlohmann::json::parse(result.at(0)["document"].c_str());
    auto tournament = std::make_shared<domain::Tournament>(rowTournament);
    tournament->Id() = std::string(result.at(0)["id"].c_str());

    return tournament;
}

std::string TournamentRepository::Create(const domain::Tournament& entity) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    const nlohmann::json tournamentBody = entity;
    pqxx::work tx(*(connection->connection));

    pqxx::result result = tx.exec(pqxx::prepped{"insert_tournament"}, tournamentBody.dump());
    tx.commit();
    return std::string(result[0]["id"].c_str());
}

std::string TournamentRepository::Update(const domain::Tournament& entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json tournamentBody = entity;

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"update_tournament"}, pqxx::params{tournamentBody.dump(), entity.Id()});
    tx.commit();

    if (result.empty()) {
        return "";
    }
    return std::string(result[0]["document"].c_str());
}

void TournamentRepository::Delete(const std::string id) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"delete_tournament"}, pqxx::params{id});
    tx.commit();
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result{tx.exec("select id, document from tournaments")};
    tx.commit();

    for (auto row : result) {
        nlohmann::json rowTournament = nlohmann::json::parse(row["document"].c_str());
        auto tournament = std::make_shared<domain::Tournament>(rowTournament);
        tournament->Id() = std::string(row["id"].c_str());

        tournaments.push_back(tournament);
    }

    return tournaments;
}