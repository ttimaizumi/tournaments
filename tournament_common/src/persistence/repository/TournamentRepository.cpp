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

#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"


TournamentRepository::TournamentRepository(std::shared_ptr<IDbConnectionProvider> connection) : connectionProvider(std::move(connection)) {
}

std::shared_ptr<domain::Tournament> TournamentRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"select_tournament_by_id"}, id);
        tx.commit();

        if (result.empty()) {
            return nullptr;
        }
        nlohmann::json rowTournament = nlohmann::json::parse(result.at(0)["document"].c_str());
        auto tournament = std::make_shared<domain::Tournament>(rowTournament);
        tournament->Id() = result.at(0)["id"].c_str();

        return tournament;
    } catch (const pqxx::data_exception& e) {
        // Handle invalid UUID format
        if (e.sqlstate() == "22P02") {
            return nullptr; // Return null for invalid ID format, let controller handle 404
        }
        throw;
    } catch (const pqxx::sql_error& e) {
        // Handle other SQL errors that might occur with invalid IDs
        if (e.sqlstate() == "22P02") {
            return nullptr; // Return null for invalid ID format, let controller handle 404
        }
        throw;
    }
}

std::string TournamentRepository::Create (const domain::Tournament & entity) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    const nlohmann::json tournamentBody = entity;
    pqxx::work tx(*(connection->connection));

    try {
        pqxx::result result = tx.exec(pqxx::prepped{"insert_tournament"}, tournamentBody.dump());
        tx.commit();
        return result[0]["id"].c_str();

    } catch (const pqxx::unique_violation& e) {
        if (e.sqlstate() == "23505") {
            throw DuplicateException("A team with the same name already exists.");
        }
        throw;
    }
}

std::string TournamentRepository::Update(const domain::Tournament &entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection *>(&*pooled);
    nlohmann::json tournamentBody = entity;

    pqxx::work tx(*(connection->connection));
    try {
        pqxx::result result = tx.exec(pqxx::prepped{"update_tournament"}, pqxx::params{ tournamentBody.dump(), entity.Id() }); 
        tx.commit();

        if (result.empty()) {
            throw NotFoundException("Tournament not found for update.");
        }
        return result[0]["document"].c_str();
    } catch (const pqxx::data_exception& e) {
        // Handle invalid UUID format
        if (e.sqlstate() == "22P02") {
            throw NotFoundException("Invalid ID format.");
        }
        throw;
    } catch (const pqxx::sql_error& e) {
        // Handle other SQL errors that might occur with invalid IDs
        if (e.sqlstate() == "22P02") {
            throw NotFoundException("Invalid ID format.");
        }
        throw;
    } catch (const std::exception& e) {
        // Log the actual error for debugging
        std::cerr << "Unexpected error in TournamentRepository::Update: " << e.what() << std::endl;
        throw;
    }
}

void TournamentRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    pqxx::work tx(*(connection->connection));
    try {
        pqxx::result result = tx.exec(pqxx::prepped{"delete_tournament"}, id);
        tx.commit();

        if (result.affected_rows() == 0) {
            throw NotFoundException("Tournament not found for deletion.");
        }
    } catch (const pqxx::data_exception& e) {
        if (e.sqlstate() == "22P02") {
            throw NotFoundException("Invalid ID format.");
        }
        throw;
    }
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result{tx.exec("select id, document from tournaments")};
    tx.commit();

    for(auto row : result){
        nlohmann::json rowTournament = nlohmann::json::parse(row["document"].c_str());
        auto tournament = std::make_shared<domain::Tournament>(rowTournament);
        tournament->Id() = row["id"].c_str();

        tournaments.push_back(tournament);
    }

    return tournaments;
}