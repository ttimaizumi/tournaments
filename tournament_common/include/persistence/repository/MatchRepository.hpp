//
// Created by root on 11/4/25.
//

#ifndef TOURNAMENTS_MATCHREPOSITORY_HPP
#define TOURNAMENTS_MATCHREPOSITORY_HPP
#include "IMatchRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"


class MatchRepository: public IMatchRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit MatchRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(connectionProvider) {}

    std::shared_ptr<domain::Match> ReadById(std::string id) override {
        return  nullptr;
    }
    std::string Create (const domain::Match & entity) override {
        return  "";
    }

    std::string Update (const domain::Match & entity) override {
        return "";
    }
    void Delete(std::string id) override {

    }

    std::vector<std::shared_ptr<domain::Match>> ReadAll() override {
        return std::vector<std::shared_ptr<domain::Match>>();
    }

    std::shared_ptr<domain::Match> FindLastOpenMatch(const std::string_view& tournamentId) override {
        return  nullptr;
    }

    std::vector<domain::Match> FindMatchesByTournamentAndRound(const std::string_view& tournamentId) override {
        return std::vector<domain::Match>();
    }
};

#endif //TOURNAMENTS_MATCHREPOSITORY_HPP