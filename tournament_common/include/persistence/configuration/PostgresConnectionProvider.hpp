//
// Created by tomas on 8/29/25.
//

#ifndef TOURNAMENTS_POSTGRESCONNECTIONPROVIDER_HPP
#define TOURNAMENTS_POSTGRESCONNECTIONPROVIDER_HPP
#include <condition_variable>
#include <queue>
#include <pqxx/pqxx>

#include "IDbConnectionProvider.hpp"
#include "PostgresConnection.hpp"

class PostgresConnectionProvider : public IDbConnectionProvider{
    std::string_view connectionString;
    size_t poolSize = 1;
    std::queue<std::unique_ptr<pqxx::connection>> connectionPool;
    std::mutex connectionPoolMutex;
    std::condition_variable connectionPoolCondition;

public:
    PostgresConnectionProvider(std::string_view connectionString, size_t poolSize) : connectionString(connectionString), poolSize(poolSize) {
        for (size_t i = 0; i < poolSize; i++) {
            connectionPool.push(std::make_unique<pqxx::connection>(connectionString.data()));
            connectionPool.back()->prepare("insert_tournament", "insert into TOURNAMENTS (document) values($1) RETURNING id");
            connectionPool.back()->prepare("select_tournament_by_id", "select * from TOURNAMENTS where id = $1");
            connectionPool.back()->prepare("insert_team", "insert into TEAMS (document) values($1) RETURNING id");
            connectionPool.back()->prepare("update_team", "UPDATE TEAMS SET document = document || $1::jsonb WHERE id = $2 RETURNING document");
            connectionPool.back()->prepare("insert_group", "insert into GROUPS (document) values($1) RETURNING id");
        }
    }

    PooledConnection Connection() override {
        std::unique_lock lock(connectionPoolMutex);

        // wait until a connection is available
        connectionPoolCondition.wait(lock, [this] { return !connectionPool.empty(); });

        // take one out
        auto conn = std::move(connectionPool.front());
        connectionPool.pop();

        // build PostgresConnection wrapper (adapts pqxx::connection -> IDbConnection)
        auto dbc = new PostgresConnection(std::move(conn));

        // return a RAII PooledConnection
        return PooledConnection(
            dbc,
            [this](IDbConnection* dbc) {
                auto pc = dynamic_cast<PostgresConnection*>(dbc);

                {
                    std::lock_guard<std::mutex> lock(connectionPoolMutex);
                    connectionPool.push(std::move(pc->connection));
                }

                delete pc;
                connectionPoolCondition.notify_one();
            }
        );
    }
};
#endif //TOURNAMENTS_POSTGRESCONNECTIONPROVIDER_HPP