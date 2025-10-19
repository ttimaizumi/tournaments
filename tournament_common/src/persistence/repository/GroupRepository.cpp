#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

#include "domain/Utilities.hpp"
#include "persistence/repository/GroupRepository.hpp"

using nlohmann::json;

namespace {
    json group_to_json(const domain::Group& g)
    {
        json j;
        j["id"]           = g.Id();
        j["name"]         = g.Name();
        j["tournamentId"] = g.TournamentId();
        j["teams"]        = json::array();
        for (const auto& t : g.Teams())
            j["teams"].push_back({ {"id", t.Id}, {"name", t.Name} });
        return j;
    }

    std::shared_ptr<domain::Group> group_from_row(const pqxx::row& r)
    {
        const auto id  = r["id"].c_str();
        const auto doc = std::string{r["document"].c_str()};
        json j = json::parse(doc);
        auto g = std::make_shared<domain::Group>(j);
        g->Id() = id;
        return g;
    }
}

GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& provider)
    : connectionProvider(provider) {}

std::shared_ptr<domain::Group> GroupRepository::ReadById(std::string id)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx{*(connection->connection)};
    auto res = tx.exec(
        pqxx::zview{"SELECT id, document FROM groups WHERE id = $1"},
        pqxx::params{id}
    );
    tx.commit();

    if (res.empty()) return nullptr;
    return group_from_row(res[0]);
}

std::string GroupRepository::Create(const domain::Group& entity)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    const json doc = group_to_json(entity);

    pqxx::work tx{*(connection->connection)};
    auto res = tx.exec(
        pqxx::zview{
            "INSERT INTO groups (tournament_id, document) "
            "VALUES ($1, $2::jsonb) "
            "RETURNING id"
        },
        pqxx::params{entity.TournamentId(), doc.dump()}
    );
    tx.commit();

    // garantizamos 1 fila
    if (res.size() != 1) throw std::runtime_error("INSERT groups RETURNING id no devolvió 1 fila");
    return res[0][0].c_str();
}

std::string GroupRepository::Update(const domain::Group& entity)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    const json doc = group_to_json(entity);

    pqxx::work tx{*(connection->connection)};
    (void)tx.exec(
        pqxx::zview{
            "UPDATE groups "
            "SET document = $1::jsonb "
            "WHERE id = $2"
        },
        pqxx::params{doc.dump(), entity.Id()}
    );
    tx.commit();

    return entity.Id();
}

void GroupRepository::Delete(std::string id)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx{*(connection->connection)};
    (void)tx.exec(
        pqxx::zview{"DELETE FROM groups WHERE id = $1"},
        pqxx::params{id}
    );
    tx.commit();
}

std::vector<std::shared_ptr<domain::Group>> GroupRepository::ReadAll()
{
    std::vector<std::shared_ptr<domain::Group>> groups;

    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx{*(connection->connection)};
    auto res = tx.exec(pqxx::zview{"SELECT id, document FROM groups"});
    tx.commit();

    groups.reserve(res.size());
    for (const auto& row : res)
        groups.push_back(group_from_row(row));

    return groups;
}

std::vector<std::shared_ptr<domain::Group>>
GroupRepository::FindByTournamentId(const std::string_view& tournamentId)
{
    std::vector<std::shared_ptr<domain::Group>> groups;

    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx{*(connection->connection)};
    auto res = tx.exec(
        pqxx::zview{"SELECT id, document FROM groups WHERE tournament_id = $1"},
        pqxx::params{std::string{tournamentId}}
    );
    tx.commit();

    groups.reserve(res.size());
    for (const auto& row : res)
        groups.push_back(group_from_row(row));

    return groups;
}

std::shared_ptr<domain::Group>
GroupRepository::FindByTournamentIdAndGroupId(const std::string_view& tournamentId,
                                              const std::string_view& groupId)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx{*(connection->connection)};
    auto res = tx.exec(
        pqxx::zview{
            "SELECT id, document FROM groups "
            "WHERE tournament_id = $1 AND id = $2"
        },
        pqxx::params{std::string{tournamentId}, std::string{groupId}}
    );
    tx.commit();

    if (res.empty()) return nullptr;
    return group_from_row(res[0]);
}

std::shared_ptr<domain::Group>
GroupRepository::FindByTournamentIdAndTeamId(const std::string_view& tournamentId,
                                             const std::string_view& teamId)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx{*(connection->connection)};
    auto res = tx.exec(
        pqxx::zview{
            "SELECT id, document "
            "FROM groups "
            "WHERE tournament_id = $1 "
            "  AND EXISTS ("
            "      SELECT 1 "
            "      FROM jsonb_array_elements(document->'teams') AS t(elem) "
            "      WHERE t.elem->>'id' = $2"
            "  )"
        },
        pqxx::params{std::string{tournamentId}, std::string{teamId}}
    );
    tx.commit();

    if (res.empty()) return nullptr;
    return group_from_row(res[0]);
}

void GroupRepository::UpdateGroupAddTeam(const std::string_view& groupId,
                                         const std::shared_ptr<domain::Team>& team)
{
    auto pooled = connectionProvider->Connection();
    auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

    json teamDoc = { {"id", team->Id}, {"name", team->Name} };

    pqxx::work tx{*(connection->connection)};
    (void)tx.exec(
        pqxx::zview{
            "UPDATE groups "
            "SET document = jsonb_set("
            "        document::jsonb,"
            "        '{teams}',"
            "        (COALESCE(document->'teams','[]'::jsonb) || $2::jsonb)"
            "    ) "
            "WHERE id = $1"
        },
        pqxx::params{std::string{groupId}, teamDoc.dump()}
    );
    tx.commit();
}
