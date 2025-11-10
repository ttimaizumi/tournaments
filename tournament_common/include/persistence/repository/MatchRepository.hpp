#ifndef TOURNAMENTS_MATCHREPOSITORY_HPP
#define TOURNAMENTS_MATCHREPOSITORY_HPP

#include <string>
#include <memory>

#include "IMatchRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "domain/Match.hpp"
#include "domain/Utilities.hpp"

class MatchRepository : public IMatchRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit MatchRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider);
    std::shared_ptr<domain::Match> ReadById(std::string id) override {
        return  nullptr;
    }
    std::string Create (const domain::Match & entity) override;
    std::string Update (const domain::Match & entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Match>> ReadAll() override {
        return std::vector<std::shared_ptr<domain::Match>>();
    }
    std::vector<std::shared_ptr<domain::Match>> FindByTournamentId(const std::string_view& tournamentId) override;
    std::shared_ptr<domain::Match> FindByTournamentIdAndMatchId(const std::string_view& tournamentId, const std::string_view& matchId) override;
    void UpdateMatchScore(const std::string_view& matchId, const domain::Score& score) override;
    std::shared_ptr<domain::Match> FindLastOpenMatch(const std::string_view& tournamentId) override {
        return  nullptr;
    }

    std::vector<domain::Match> FindMatchesByTournamentAndRound(const std::string_view& tournamentId) override {
        return std::vector<domain::Match>();
    }
};

#endif //TOURNAMENTS_MATCHREPOSITORY_HPP