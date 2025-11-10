//
// Created by root on 11/4/25.
//

#ifndef TOURNAMENTS_MATCHREPOSITORY_HPP
#define TOURNAMENTS_MATCHREPOSITORY_HPP
#include "IMatchRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>

class MatchRepository: public IMatchRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit MatchRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider)
        : connectionProvider(connectionProvider) {}

    std::shared_ptr<domain::Match> ReadById(std::string id) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params("SELECT id, document FROM matches WHERE id = $1", id);

        if (result.empty()) {
            tx.commit();
            return nullptr;
        }

        tx.commit();

        auto match = std::make_shared<domain::Match>();
        nlohmann::json doc = nlohmann::json::parse(result[0]["document"].c_str());
        *match = doc.get<domain::Match>();
        match->SetId(result[0]["id"].c_str());

        return match;
    }

    std::string Create(const domain::Match& entity) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        nlohmann::json matchBody = entity;

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params(
            "INSERT INTO matches (document) VALUES ($1) RETURNING id",
            matchBody.dump()
        );
        tx.commit();

        return result[0]["id"].c_str();
    }

    std::string Update(const domain::Match& entity) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        nlohmann::json matchBody = entity;

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params(
            "UPDATE matches SET document = $1, last_update_date = CURRENT_TIMESTAMP WHERE id = $2 RETURNING id",
            matchBody.dump(), entity.Id()
        );
        tx.commit();

        return result[0]["id"].c_str();
    }

    void Delete(std::string id) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        tx.exec_params("DELETE FROM matches WHERE id = $1", id);
        tx.commit();
    }

    std::vector<std::shared_ptr<domain::Match>> ReadAll() override {
        std::vector<std::shared_ptr<domain::Match>> matches;
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result{tx.exec("SELECT id, document FROM matches")};
        tx.commit();

        for(auto row : result) {
            auto match = std::make_shared<domain::Match>();
            nlohmann::json doc = nlohmann::json::parse(row["document"].c_str());
            *match = doc.get<domain::Match>();
            match->SetId(row["id"].c_str());
            matches.push_back(match);
        }

        return matches;
    }

    std::vector<std::shared_ptr<domain::Match>> FindByTournamentId(const std::string_view& tournamentId) override {
        std::vector<std::shared_ptr<domain::Match>> matches;
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params(
            "SELECT id, document FROM matches WHERE document->>'tournamentId' = $1",
            tournamentId.data()
        );
        tx.commit();

        for(auto row : result) {
            auto match = std::make_shared<domain::Match>();
            nlohmann::json doc = nlohmann::json::parse(row["document"].c_str());
            *match = doc.get<domain::Match>();
            match->SetId(row["id"].c_str());
            matches.push_back(match);
        }

        return matches;
    }

    std::vector<std::shared_ptr<domain::Match>> FindPlayedByTournamentId(const std::string_view& tournamentId) override {
        std::vector<std::shared_ptr<domain::Match>> matches;
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params(
            "SELECT id, document FROM matches WHERE document->>'tournamentId' = $1 AND document ? 'score'",
            tournamentId.data()
        );
        tx.commit();

        for(auto row : result) {
            auto match = std::make_shared<domain::Match>();
            nlohmann::json doc = nlohmann::json::parse(row["document"].c_str());
            *match = doc.get<domain::Match>();
            match->SetId(row["id"].c_str());
            matches.push_back(match);
        }

        return matches;
    }

    std::vector<std::shared_ptr<domain::Match>> FindPendingByTournamentId(const std::string_view& tournamentId) override {
        std::vector<std::shared_ptr<domain::Match>> matches;
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params(
            "SELECT id, document FROM matches WHERE document->>'tournamentId' = $1 AND NOT (document ? 'score')",
            tournamentId.data()
        );
        tx.commit();

        for(auto row : result) {
            auto match = std::make_shared<domain::Match>();
            nlohmann::json doc = nlohmann::json::parse(row["document"].c_str());
            *match = doc.get<domain::Match>();
            match->SetId(row["id"].c_str());
            matches.push_back(match);
        }

        return matches;
    }

    std::shared_ptr<domain::Match> FindByTournamentIdAndMatchId(const std::string_view& tournamentId, const std::string_view& matchId) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params(
            "SELECT id, document FROM matches WHERE id = $1 AND document->>'tournamentId' = $2",
            matchId.data(), tournamentId.data()
        );

        if (result.empty()) {
            tx.commit();
            return nullptr;
        }

        tx.commit();

        auto match = std::make_shared<domain::Match>();
        nlohmann::json doc = nlohmann::json::parse(result[0]["document"].c_str());
        *match = doc.get<domain::Match>();
        match->SetId(result[0]["id"].c_str());

        return match;
    }

    std::shared_ptr<domain::Match> FindLastOpenMatch(const std::string_view& tournamentId) override {
        // This method is for finding incomplete matches during team assignment
        // Not needed for MUNDIAL implementation
        return nullptr;
    }

    std::vector<std::shared_ptr<domain::Match>> FindMatchesByTournamentAndRound(const std::string_view& tournamentId, domain::Round round) override {
        std::vector<std::shared_ptr<domain::Match>> matches;
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        std::string roundStr = domain::roundToString(round);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params(
            "SELECT id, document FROM matches WHERE document->>'tournamentId' = $1 AND document->>'round' = $2",
            tournamentId.data(), roundStr
        );
        tx.commit();

        for(auto row : result) {
            auto match = std::make_shared<domain::Match>();
            nlohmann::json doc = nlohmann::json::parse(row["document"].c_str());
            *match = doc.get<domain::Match>();
            match->SetId(row["id"].c_str());
            matches.push_back(match);
        }

        return matches;
    }

    bool TournamentExists(const std::string_view& tournamentId) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec_params(
            "SELECT COUNT(*) as count FROM tournaments WHERE id = $1",
            tournamentId.data()
        );
        tx.commit();

        return result[0]["count"].as<int>() > 0;
    }
};

#endif //TOURNAMENTS_MATCHREPOSITORY_HPP