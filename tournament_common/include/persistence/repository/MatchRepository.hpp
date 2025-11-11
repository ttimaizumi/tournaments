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
    std::vector<std::shared_ptr<domain::Match>> FindByTournamentId(const std::string_view& tournamentId) override;
    std::shared_ptr<domain::Match> FindByTournamentIdAndMatchId(const std::string_view& tournamentId, const std::string_view& matchId) override;
    void UpdateMatchScore(const std::string_view& matchId, const domain::Score& score) override;
    std::vector<std::string> CreateBulk(const std::vector<domain::Match>& matches) override;
    bool MatchesExistForTournament(const std::string_view& tournamentId) override;
};

#endif //TOURNAMENTS_MATCHREPOSITORY_HPP