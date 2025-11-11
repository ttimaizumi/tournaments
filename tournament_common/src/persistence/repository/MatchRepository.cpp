#include "domain/Utilities.hpp"
#include  "persistence/repository/MatchRepository.hpp"

MatchRepository::MatchRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

std::vector<std::shared_ptr<domain::Match>> MatchRepository::FindByTournamentId(const std::string_view& tournamentId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"select_matches_by_tournament"}, pqxx::params{tournamentId.data()});
    tx.commit();

    std::vector<std::shared_ptr<domain::Match>> matches;
    for(auto row : result){
        nlohmann::json matchDocument = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>(matchDocument);
        match->Id() = row["id"].c_str();

        matches.push_back(match);
    }

    return matches;
}

std::shared_ptr<domain::Match> MatchRepository::FindByTournamentIdAndMatchId(const std::string_view& tournamentId, const std::string_view& matchId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"select_match_by_tournamentid_matchid"}, pqxx::params{tournamentId.data(), matchId.data()});
    tx.commit();
    if (result.empty()) {
        return nullptr;
    }
    nlohmann::json matchDocument = nlohmann::json::parse(result[0]["document"].c_str());
    auto match = std::make_shared<domain::Match>(matchDocument);
    match->Id() = result[0]["id"].c_str();

    return match;
}

void MatchRepository::UpdateMatchScore(const std::string_view& matchId, const domain::Score& score) {
    nlohmann::json scoreDocument = score;
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"update_match_score"}, pqxx::params{matchId.data(), scoreDocument.dump()});
    tx.commit();
}

std::vector<std::string> MatchRepository::CreateBulk(const std::vector<domain::Match>& matches) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    std::vector<std::string> createdIds;
    for (const auto& match : matches) {
        nlohmann::json matchDocument = match;
        const pqxx::result result = tx.exec(pqxx::prepped{"insert_match"}, pqxx::params{match.TournamentId().data(),
                                                                                      matchDocument.dump()});
        createdIds.push_back(result[0]["id"].c_str());
    }
    tx.commit();
    return createdIds;
}

bool MatchRepository::MatchesExistForTournament(const std::string_view& tournamentId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"select_matches_by_tournament"}, pqxx::params{tournamentId.data()});
    tx.commit();

    return !result.empty();
}