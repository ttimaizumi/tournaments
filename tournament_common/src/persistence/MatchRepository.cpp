//
// Created by edgar on 11/10/25.
//#include "persistence/repository/MatchRepository.hpp"

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

using nlohmann::json;

MatchRepository::MatchRepository(std::shared_ptr<PostgresConnectionProvider> provider)
    : connectionProvider(std::move(provider))
{
}

std::vector<json>
MatchRepository::FindByTournament(std::string_view tournamentId,
                                  std::optional<std::string> statusFilter)
{
    std::vector<json> matches;

    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx{*(connection->connection)};

    // document->>'tournamentId' se guarda dentro del JSON de MATCHES
    std::string sql =
        "SELECT id, document "
        "FROM matches "
        "WHERE document->>'tournamentId' = $1";

    if (statusFilter.has_value())
    {
        sql += " AND document->>'status' = $2";
    }

    pqxx::result res;
    if (statusFilter.has_value())
    {
        res = tx.exec_params(sql, std::string{tournamentId}, statusFilter.value());
    }
    else
    {
        res = tx.exec_params(sql, std::string{tournamentId});
    }

    tx.commit();

    matches.reserve(res.size());
    for (const auto& row : res)
    {
        json doc = json::parse(row["document"].c_str());
        // opcional: incluye el id en la respuesta
        doc["id"] = row["id"].c_str();
        matches.push_back(std::move(doc));
    }

    return matches;
}

std::optional<json>
MatchRepository::FindByTournamentAndId(std::string_view tournamentId,
                                       std::string_view matchId)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx{*(connection->connection)};

    auto res = tx.exec_params(
        "SELECT id, document "
        "FROM matches "
        "WHERE id = $1 "
        "  AND document->>'tournamentId' = $2",
        std::string{matchId},
        std::string{tournamentId});

    tx.commit();

    if (res.empty())
        return std::nullopt;

    json doc = json::parse(res[0]["document"].c_str());
    doc["id"] = res[0]["id"].c_str();
    return doc;
}

bool MatchRepository::UpdateScore(std::string_view tournamentId,
                                  std::string_view matchId,
                                  const json& newScore,
                                  std::string newStatus)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx{*(connection->connection)};

    // actualiza document.score y document.status
    auto res = tx.exec_params(
        "UPDATE matches "
        "SET document = jsonb_set("
        "        jsonb_set(document, '{score}', $1::jsonb, true),"
        "        '{status}', to_jsonb($2::text), true"
        "    ),"
        "    last_update_date = CURRENT_TIMESTAMP "
        "WHERE id = $3 "
        "  AND document->>'tournamentId' = $4 "
        "RETURNING id",
        newScore.dump(),
        newStatus,
        std::string{matchId},
        std::string{tournamentId});

    tx.commit();

    return !res.empty();
}
