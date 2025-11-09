//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>

#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "RunConfiguration.hpp"
#include "cms/ConnectionManager.hpp"
#include "delegate/TeamDelegate.hpp"
#include "controller/HealthController.hpp"
#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "cms/QueueResolver.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "../delegate/GroupDelegate.hpp"
#include "controller/GroupController.hpp"

namespace config {
    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        std::ifstream file("configuration.json");
        nlohmann::json configuration;
        file >> configuration;
        std::shared_ptr<RunConfiguration> appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
        builder.registerInstance(appConfig);

        std::shared_ptr<PostgresConnectionProvider> postgressConnection = std::make_shared<PostgresConnectionProvider>(
            configuration["databaseConfig"]["connectionString"].get<std::string>(),
            configuration["databaseConfig"]["poolSize"].get<size_t>());
        builder.registerInstance(postgressConnection).as<IDbConnectionProvider>();

        builder.registerType<ConnectionManager>()
            .onActivated([configuration](Hypodermic::ComponentContext&, const std::shared_ptr<ConnectionManager>& instance) {
                instance->initialize(configuration["activemq"]["broker-url"].get<std::string>());
            })
            .singleInstance();

        builder.registerType<QueueMessageProducer>().as<IQueueMessageProducer>().named("tournamentAddTeamQueue");
        builder.registerType<QueueResolver>().as<IResolver<IQueueMessageProducer> >().named("queueResolver").
                singleInstance();

        builder.registerType<TeamRepository>().as<IRepository<domain::Team, std::string_view> >().singleInstance();
        builder.registerType<GroupRepository>().as<IGroupRepository>().singleInstance();

        builder.registerType<TeamDelegate>().as<ITeamDelegate>().singleInstance();
        builder.registerType<TeamController>().singleInstance();

        builder.registerType<TournamentRepository>().as<IRepository<domain::Tournament, std::string> >().
                singleInstance();

        builder.registerType<TournamentDelegate>()
                .as<ITournamentDelegate>()
                .singleInstance();
        builder.registerType<TournamentController>().singleInstance();

        builder.registerType<GroupDelegate>().as<IGroupDelegate>()
            .with<IQueueMessageProducer>([](Hypodermic::ComponentContext& context){
                return context.resolveNamed<QueueMessageProducer>("tournamentAddTeamQueue");
            })
            .singleInstance();
        builder.registerType<GroupController>().singleInstance();
        builder.registerType<HealthController>().singleInstance();

        return builder.build();
    }
}
#endif //RESTAPI_CONTAINER_SETUP_HPP
