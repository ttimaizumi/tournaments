#include "persistence/repository/TeamRepository.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "domain/Utilities.hpp"
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
  return std::make_shared<domain::Team>();
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
  return "newID";
}

void TeamRepository::Delete(std::string_view id) {}