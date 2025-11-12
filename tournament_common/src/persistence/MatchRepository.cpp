//
// Created by edgar on 11/10/25.
//

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include "persistence/configuration/PostgresConnection.hpp"
#include "persistence/repository/MatchRepository.hpp"

using nlohmann::json;

static pqxx::work make_tx(PostgresConnection* pc) {
    // usa parentesis (no llaves) para evitar la sobrecarga equivocada
    return pqxx::work(*(pc->connection));
}

std::vector<json>
MatchRepository::FindByTournament(std::string_view tournamentId,
                                  std::optional<std::string> statusFilter) {
    auto pooled = dbProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx = make_tx(connection);

    pqxx::result res;
    if (statusFilter.has_value()) {
        res = tx.exec_params(
            "SELECT id, document "
            "FROM matches "
            "WHERE document->>'tournamentId' = $1 "
            "  AND document->>'status' = $2 "
            "ORDER BY created_at",
            std::string{tournamentId},
            statusFilter.value());
    } else {
        res = tx.exec_params(
            "SELECT id, document "
            "FROM matches "
            "WHERE document->>'tournamentId' = $1 "
            "ORDER BY created_at",
            std::string{tournamentId});
    }
    tx.commit();

    std::vector<json> matches;
    matches.reserve(res.size());
    for (const auto& row : res) {
        json doc = json::parse(row["document"].c_str());
        doc["id"] = row["id"].c_str();
        matches.push_back(std::move(doc));
    }
    return matches;
}

std::optional<json>
MatchRepository::FindByTournamentAndId(std::string_view tournamentId,
                                       std::string_view matchId) {
    auto pooled = dbProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx = make_tx(connection);

    auto res = tx.exec_params(
        "SELECT id, document "
        "FROM matches "
        "WHERE id = $1 "
        "  AND document->>'tournamentId' = $2",
        std::string{matchId},
        std::string{tournamentId});

    tx.commit();

    if (res.empty()) return std::nullopt;

    json doc = json::parse(res[0]["document"].c_str());
    doc["id"] = res[0]["id"].c_str();
    return doc;
}

std::optional<std::string>
MatchRepository::Create(const json& matchDocument) {
    auto pooled = dbProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx = make_tx(connection);

    auto res = tx.exec_params(
        "INSERT INTO matches (document) "
        "VALUES ($1::jsonb) "
        "RETURNING id",
        matchDocument.dump());

    tx.commit();

    if (res.empty()) return std::nullopt;
    return std::optional<std::string>{res[0]["id"].c_str()};
}

bool
MatchRepository::UpdateScore(std::string_view tournamentId,
                             std::string_view matchId,
                             const json& newScore,
                             std::string newStatus) {
    auto pooled = dbProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx = make_tx(connection);

    auto res = tx.exec_params(
        "UPDATE matches "
        "SET document = jsonb_set("
        "        jsonb_set(document, '{score}', $1::jsonb, true), "
        "        '{status}', to_jsonb($2::text), true"
        "    ), "
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

bool
MatchRepository::UpdateParticipants(std::string_view tournamentId,
                                    std::string_view matchId,
                                    std::optional<std::string> homeTeamId,
                                    std::optional<std::string> visitorTeamId) {
    auto pooled = dbProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx = make_tx(connection);

    // Construimos el set jsonb dinamico
    std::string setExpr = "document";
    std::vector<std::string> params;

    if (homeTeamId.has_value()) {
        setExpr = "jsonb_set(" + setExpr + ", '{homeTeamId}', to_jsonb($1::text), true)";
        params.push_back(homeTeamId.value());
    }
    if (visitorTeamId.has_value()) {
        const std::string idx = std::to_string(params.size() + 1);
        setExpr = "jsonb_set(" + setExpr + ", '{visitorTeamId}', to_jsonb($" + idx + "::text), true)";
        params.push_back(visitorTeamId.value());
    }

    const std::string midx = std::to_string(params.size() + 1);
    const std::string tidx = std::to_string(params.size() + 2);

    std::string sql =
        "UPDATE matches "
        "SET document = " + setExpr + ", "
        "    last_update_date = CURRENT_TIMESTAMP "
        "WHERE id = $" + midx + " "
        "  AND document->>'tournamentId' = $" + tidx + " "
        "RETURNING id";

    // Construye params tipados para libpqxx moderno
    pqxx::params p;
    for (auto& s : params) p.append(s);
    std::string matchIdStr{matchId};
    std::string tournStr{tournamentId};
    p.append(matchIdStr);
    p.append(tournStr);

    pqxx::result res = tx.exec(sql, p);
    tx.commit();

    return !res.empty();
}
