//
// Created by tomas on 9/7/25.
//
#ifndef TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP
#define TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>
#include <print>

#include "configuration/DatabaseConfiguration.hpp"
#include "cms/ConnectionManager.hpp"
#include "cms/QueueMessageListener.hpp"
#include "cms/GroupAddTeamListener.hpp"
#include "cms/ScoreRecordedListener.hpp"
#include "cms/TournamentFullListener.hpp"


#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"

#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"

// match repo
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/MatchRepository.hpp"

// group repo
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"

namespace config {

    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        std::ifstream file("configuration.json");
        nlohmann::json configuration;
        file >> configuration;

        // DB provider
        auto pg = std::make_shared<PostgresConnectionProvider>(
            configuration["databaseConfig"]["connectionString"].get<std::string>(),
            configuration["databaseConfig"]["poolSize"].get<size_t>());
        builder.registerInstance(pg).as<IDbConnectionProvider>();

        // ActiveMQ
        builder.registerType<ConnectionManager>()
            .onActivated([configuration](Hypodermic::ComponentContext&,
                                         const std::shared_ptr<ConnectionManager>& instance) {
                instance->initialize(configuration["activemq"]["broker-url"].get<std::string>());
            })
            .singleInstance();

        // repos
        builder.registerType<TeamRepository>()
            .as<IRepository<domain::Team, std::string_view>>()
            .singleInstance();

        builder.registerType<TournamentRepository>()
            .as<IRepository<domain::Tournament, std::string>>()
            .singleInstance();

        builder.registerType<MatchRepository>()
            .as<IMatchRepository>()
            .singleInstance();

        builder.registerType<GroupRepository>()
           .as<IGroupRepository>()
           .singleInstance();

        // listeners
        builder.registerType<GroupAddTeamListener>();
        builder.registerType<ScoreRecordedListener>();
        builder.registerType<TournamentFullListener>();


        return builder.build();
    }

}
#endif //TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP
