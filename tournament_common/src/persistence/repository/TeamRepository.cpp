#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

#include "domain/Utilities.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"

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
  auto pooled = connectionProvider->Connection();
  const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

  pqxx::work tx(*(connection->connection));
  const pqxx::result result = tx.exec(pqxx::prepped{"select_team_by_id"}, pqxx::params{id});
  tx.commit();
  if (result.empty()) {
    return nullptr;
  }

  nlohmann::json rowTeam = nlohmann::json::parse(result.at(0)["document"].c_str());
  auto team = std::make_shared<domain::Team>(rowTeam);
  team->Id = result.at(0)["id"].c_str();

  return team;
}

std::string_view TeamRepository::Create(const domain::Team &entity) {
  auto pooled = connectionProvider->Connection();
  auto connection = dynamic_cast<PostgresConnection *>(&*pooled);
  nlohmann::json teamBody = entity;

  pqxx::work tx(*(connection->connection));
  pqxx::result result = tx.exec(pqxx::prepped{"insert_team"}, teamBody.dump());
  tx.commit();
  
  return result[0]["id"].c_str();
}

std::string_view TeamRepository::Update(const domain::Team &entity) {
  auto pooled = connectionProvider->Connection();
  auto connection = dynamic_cast<PostgresConnection *>(&*pooled);
  nlohmann::json teamBody = entity;

  pqxx::work tx(*(connection->connection));
  pqxx::result result = tx.exec(pqxx::prepped{"update_team"}, pqxx::params{ teamBody.dump(), entity.Id });
  tx.commit();
  return result[0]["document"].c_str();
}

void TeamRepository::Delete(std::string_view id) {
  auto pooled = connectionProvider->Connection();
  auto connection = dynamic_cast<PostgresConnection *>(&*pooled);

  pqxx::work tx(*(connection->connection));
  pqxx::result result = tx.exec(pqxx::prepped{"delete_team"}, pqxx::params{id});
  tx.commit();
}