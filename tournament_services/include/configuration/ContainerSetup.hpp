#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <string_view>

// Repos base
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"

// Match repo
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/MatchRepository.hpp"

// DB
#include "persistence/configuration/PostgresConnectionProvider.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"

// CMS / ActiveMQ
#include "cms/IQueueMessageProducer.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "cms/ConnectionManager.hpp"

// Delegates
#include "delegate/TeamDelegate.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "delegate/GroupDelegate.hpp"
#include "delegate/IMatchDelegate.hpp"
#include "delegate/MatchDelegate.hpp"

// Controllers
#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "controller/GroupController.hpp"
#include "controller/MatchController.hpp"

// Run config
#include "configuration/RunConfiguration.hpp"

namespace config {

    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        // Configuracion
        std::ifstream file("configuration.json");
        nlohmann::json configuration;
        file >> configuration;

        auto appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
        builder.registerInstance(appConfig);

        // Postgres provider
        auto pg = std::make_shared<PostgresConnectionProvider>(
            configuration["databaseConfig"]["connectionString"].get<std::string>(),
            configuration["databaseConfig"]["poolSize"].get<size_t>());

        builder.registerInstance(pg).as<IDbConnectionProvider>();

        // ActiveMQ connection manager + producer
        builder.registerType<ConnectionManager>()
            .onActivated([configuration](Hypodermic::ComponentContext&,
                                         const std::shared_ptr<ConnectionManager>& instance) {
                instance->initialize(configuration["activemq"]["broker-url"].get<std::string>());
            })
            .singleInstance();

        builder.registerType<QueueMessageProducer>()
            .as<IQueueMessageProducer>()
            .singleInstance();

        // Repos
        builder.registerType<TeamRepository>()
            .as<IRepository<domain::Team, std::string_view>>()
            .singleInstance();

        builder.registerType<TournamentRepository>()
            .as<IRepository<domain::Tournament, std::string>>()
            .singleInstance();

        builder.registerType<GroupRepository>()
            .singleInstance();

        builder.registerType<MatchRepository>()
            .as<IMatchRepository>()
            .singleInstance();

        // Delegates
        builder.registerType<TeamDelegate>().as<ITeamDelegate>().singleInstance();
        builder.registerType<TournamentDelegate>().as<ITournamentDelegate>().singleInstance();
        builder.registerType<GroupDelegate>().as<IGroupDelegate>().singleInstance();
        builder.registerType<MatchDelegate>().as<IMatchDelegate>().singleInstance();

        // Controllers
        builder.registerType<TeamController>().singleInstance();
        builder.registerType<TournamentController>().singleInstance();
        builder.registerType<GroupController>().singleInstance();
        builder.registerType<MatchController>().singleInstance();

        return builder.build();
    }

} // namespace config

#endif // RESTAPI_CONTAINER_SETUP_HPP
