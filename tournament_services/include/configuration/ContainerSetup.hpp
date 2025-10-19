#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <string_view>

#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"

#include "persistence/configuration/PostgresConnectionProvider.hpp"

#include "cms/IQueueMessageProducer.hpp"
#include "cms/QueueMessageProducer.hpp"

#include "delegate/TeamDelegate.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "delegate/GroupDelegate.hpp"

#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "controller/GroupController.hpp"

#include "RunConfiguration.hpp"

namespace config {

inline std::shared_ptr<Hypodermic::Container> containerSetup() {
    Hypodermic::ContainerBuilder builder;

    // config
    std::ifstream file("configuration.json");
    nlohmann::json configuration;
    file >> configuration;

    auto appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
    builder.registerInstance(appConfig);

    auto pg = std::make_shared<PostgresConnectionProvider>(
        configuration["databaseConfig"]["connectionString"].get<std::string>(),
        configuration["databaseConfig"]["poolSize"].get<size_t>());
    builder.registerInstance(pg).as<IDbConnectionProvider>();

    // Producer de colas (inyectar como interfaz)
    builder.registerType<QueueMessageProducer>()
           .as<IQueueMessageProducer>()
           .singleInstance();

    // Repos
    builder.registerType<TournamentRepository>()
           .as<IRepository<domain::Tournament, std::string>>()  // Tournament usa std::string
           .singleInstance();

    builder.registerType<TeamRepository>()
           .as<IRepository<domain::Team, std::string_view>>()   // Team usa std::string_view
           .singleInstance();

    builder.registerType<GroupRepository>()
           .as<IGroupRepository>()
           .singleInstance();

    // Delegates
    builder.registerType<TeamDelegate>()
           .as<ITeamDelegate>()
           .singleInstance();

    builder.registerType<TournamentDelegate>()
           .as<ITournamentDelegate>()
           .singleInstance();

    builder.registerType<GroupDelegate>()
           .as<IGroupDelegate>()
           .singleInstance();

    // Controllers
    builder.registerType<TeamController>().singleInstance();
    builder.registerType<TournamentController>().singleInstance();
    builder.registerType<GroupController>().singleInstance();

    return builder.build();
}

} // namespace config
#endif // RESTAPI_CONTAINER_SETUP_HPP
