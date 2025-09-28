//
// Created by root on 9/21/25.
//

#ifndef TOURNAMENTS_GROUPREPOSITORY_HPP
#define TOURNAMENTS_GROUPREPOSITORY_HPP

#include <string>
#include <memory>

#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "IRepository.hpp"
#include "domain/Group.hpp"
#include "domain/Utilities.hpp"

class GroupRepository : public IRepository<domain::Group, std::string>{
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider);
    std::shared_ptr<domain::Group> ReadById(std::string id) override;
    std::string Create (const domain::Group & entity) override;
    std::string Update (const domain::Group & entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Group>> ReadAll() override;
};

GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

std::shared_ptr<domain::Group> GroupRepository::ReadById(std::string id) {
    return std::make_shared<domain::Group>();
}

std::string GroupRepository::Create (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json groupBody = entity;

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"insert_group"}, groupBody.dump());

    tx.commit();

    return result[0]["id"].c_str();
}

std::string GroupRepository::Update (const domain::Group & entity) {
    return "update-id";
}

void GroupRepository::Delete(std::string id) {

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

#endif //TOURNAMENTS_GROUPREPOSITORY_HPP