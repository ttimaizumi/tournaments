#include "persistence/repository/TeamRepository.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

#include "domain/Utilities.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"

TeamRepository::TeamRepository(
    std::shared_ptr<IDbConnectionProvider> connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

std::vector<std::shared_ptr<domain::Team>> TeamRepository::ReadAll() {
  std::vector<std::shared_ptr<domain::Team>> teams;

  auto pooled = connectionProvider->Connection();
  auto connection = dynamic_cast<PostgresConnection *>(&*pooled);

  pqxx::work tx(*(connection->connection));
  pqxx::result result{
      tx.exec("select id, document->>'name' as name from teams")};
  tx.commit();

  for (auto row : result) {
    teams.push_back(std::make_shared<domain::Team>(
        domain::Team{row["id"].c_str(), row["name"].c_str()}));
  }

  return teams;
}

std::shared_ptr<domain::Team> TeamRepository::ReadById(std::string_view id) {
  return std::make_shared<domain::Team>();
}

std::string_view TeamRepository::Create(const domain::Team &entity) {
  auto pooled = connectionProvider->Connection();
  auto connection = dynamic_cast<PostgresConnection *>(&*pooled);
  nlohmann::json teamBody = entity;

  pqxx::work tx(*(connection->connection));
  try {
    pqxx::result result = tx.exec(pqxx::prepped{"insert_team"}, teamBody.dump());
    tx.commit();
    return result[0]["id"].c_str();

  } catch (const pqxx::unique_violation& e) {
    if (e.sqlstate() == "23505") {
        throw DuplicateException("A team with the same name already exists.");
    }
    throw;
  }
}

std::string_view TeamRepository::Update(const domain::Team &entity) {
  auto pooled = connectionProvider->Connection();
  auto connection = dynamic_cast<PostgresConnection *>(&*pooled);
  nlohmann::json teamBody = entity;

  pqxx::work tx(*(connection->connection));
  try {
    pqxx::result result = tx.exec(pqxx::prepped{"update_team"}, pqxx::params{ teamBody.dump(), entity.Id }); // Should catch if url id is non uuid? or keep the 500
    tx.commit();
    // No rows were updated, meaning the team with the given ID does not exist
    if (result.empty()) {
        throw NotFoundException("Team not found for update.");
    }
    return result[0]["document"].c_str();

  } catch (const pqxx::data_exception& e) {
    if (e.sqlstate() == "22P02") {
        throw InvalidFormatException("Invalid ID format.");
    }
    throw;
  }
}

void TeamRepository::Delete(std::string_view id) {
  auto pooled = connectionProvider->Connection();
  auto connection = dynamic_cast<PostgresConnection *>(&*pooled);

  pqxx::work tx(*(connection->connection));
  try {
    pqxx::result result = tx.exec(pqxx::prepped{"delete_team"}, pqxx::params{id});
    tx.commit();
    if (result.empty()) {
        throw NotFoundException("Team not found for deletion.");
    }
  } catch (const pqxx::data_exception& e) {
    if (e.sqlstate() == "22P02") {
      throw InvalidFormatException("Invalid ID format.");
    }
    throw;
  }
}