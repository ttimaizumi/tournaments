//
// Created by root on 9/27/25.
//

#include "domain/Utilities.hpp"
#include  "persistence/repository/GroupRepository.hpp"

#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"
#include "exception/Duplicate.hpp"

GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

std::vector<std::shared_ptr<domain::Group>> GroupRepository::FindByTournamentId(const std::string_view& tournamentId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    try {
        pqxx::result result = tx.exec(pqxx::prepped{"select_groups_by_tournament"}, pqxx::params{tournamentId.data()});
        tx.commit();

    std::vector<std::shared_ptr<domain::Group>> groups;
    for(auto row : result){
        nlohmann::json groupDocument = nlohmann::json::parse(row["document"].c_str());
        auto group = std::make_shared<domain::Group>(groupDocument);
        group->Id() = row["id"].c_str();

        groups.push_back(group);
    }
    return groups;
    } catch (const pqxx::data_exception& e) {
        // Handle invalid UUID format
        if (e.sqlstate() == "22P02") {
            throw InvalidFormatException("Invalid ID format.");
        }
        throw;
    }
}

std::shared_ptr<domain::Group> GroupRepository::ReadById(std::string id) {
    return std::make_shared<domain::Group>();
}

std::string GroupRepository::Create (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json groupBody = entity;

    pqxx::work tx(*(connection->connection));
    try {
    pqxx::result result = tx.exec(pqxx::prepped{"insert_group"}, pqxx::params{entity.TournamentId(), groupBody.dump()});
    tx.commit();
    if (result.empty()) {
        throw std::runtime_error("Failed to insert group.");
    }
    return result[0]["id"].c_str();
    } catch (const pqxx::unique_violation& e) {
        if (e.sqlstate() == "23505") {
            throw DuplicateException("A group with the same name already exists in this tournament.");
        }
        throw;
    }
}

std::string GroupRepository::Update (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json groupBody = entity;

    pqxx::work tx(*(connection->connection));
    try {
        pqxx::result result = tx.exec(pqxx::prepped{"update_group"}, pqxx::params{entity.Id(), groupBody.dump()});
        tx.commit();

        if (result.empty()) {
            throw NotFoundException("Group not found for update.");
        }
        return result[0]["document"].c_str();
    } catch (const pqxx::data_exception& e) {
        // Handle invalid UUID format
        if (e.sqlstate() == "22P02") {
            throw InvalidFormatException("Invalid group ID format.");
        }
        throw;
    } catch (const pqxx::unique_violation& e) {
        if (e.sqlstate() == "23505") {
            throw DuplicateException("A group with the same name already exists in this tournament.");
        }
        throw;
    }
}

void GroupRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    try {
        pqxx::result result = tx.exec(pqxx::prepped{"delete_group"}, pqxx::params{id});
        tx.commit();

        if (result.empty()) {
            throw NotFoundException("Group not found for deletion.");
        }
    } catch (const pqxx::data_exception& e) {
        // Handle invalid UUID format
        if (e.sqlstate() == "22P02") {
            throw InvalidFormatException("Invalid group ID format.");
        }
        throw;
    }
}

std::vector<std::shared_ptr<domain::Group>> GroupRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Group>> teams;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result{tx.exec("select id, document->>'name' as name from groups")};
    tx.commit();

    for(auto row : result){
        teams.push_back(std::make_shared<domain::Group>(domain::Group{row["id"].c_str(), row["name"].c_str()}));
    }

    return teams;
}

std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"select_group_by_tournamentid_groupid"}, pqxx::params{tournamentId.data(), groupId.data()});
        tx.commit();

        if (result.empty()) {
            throw NotFoundException("Group not found in this tournament.");
        }
        nlohmann::json groupDocument = nlohmann::json::parse(result[0]["document"].c_str());
        auto group = std::make_shared<domain::Group>(groupDocument);
        group->Id() = result[0]["id"].c_str();
        return group;
    } catch (const pqxx::data_exception& e) {
        // Handle invalid UUID format
        if (e.sqlstate() == "22P02") {
            throw InvalidFormatException("Invalid group ID format.");
        }
        throw;
    }
}

std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"select_group_in_tournament"}, pqxx::params{tournamentId.data(), teamId.data()});
    tx.commit();
    if (result.empty()) {
        return nullptr;
    }
    nlohmann::json groupDocument = nlohmann::json::parse(result[0]["document"].c_str());
    std::shared_ptr<domain::Group> group = std::make_shared<domain::Group>(groupDocument);
    group->Id() = result[0]["id"].c_str();

    return group;
}

void GroupRepository::UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team> & team) {
    nlohmann::json teamDocument = team;
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"update_group_add_team"}, pqxx::params{groupId.data(), teamDocument.dump()});
    tx.commit();
}