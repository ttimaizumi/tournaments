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

// Nuevo: repos de matches
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/MatchRepository.hpp"

// DB
#include "persistence/configuration/PostgresConnectionProvider.hpp"

// CMS / ActiveMQ
#include "cms/IQueueMessageProducer.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "cms/ConnectionManager.hpp"

// Delegates existentes
#include "delegate/TeamDelegate.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "delegate/GroupDelegate.hpp"

// Nuevo: delegate de matches
#include "delegate/IMatchDelegate.hpp"
#include "delegate/MatchDelegate.hpp"

// Controllers existentes
#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "controller/GroupController.hpp"

// Nuevo: controller de matches
#include "controller/MatchController.hpp"

#include "RunConfiguration.hpp"

namespace config {

inline std::shared_ptr<Hypodermic::Container> containerSetup()
{
    Hypodermic::ContainerBuilder builder;

    // =========================
    // Configuracion de la app
    // =========================
    std::ifstream file("configuration.json");
    nlohmann::json configuration;
    file >> configuration;

    auto appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
    builder.registerInstance(appConfig);

    // =========================
    // Postgres
    // =========================
    auto pg = std::make_shared<PostgresConnectionProvider>(
        configuration["databaseConfig"]["connectionString"].get<std::string>(),
        configuration["databaseConfig"]["poolSize"].get<size_t>());

    // Se registra como IDbConnectionProvider (asi la usan los repos)
    builder.registerInstance(pg).as<IDbConnectionProvider>();

    // =========================
    // ActiveMQ / ConnectionManager
    // =========================
    builder.registerType<ConnectionManager>()
        .onActivated(
            [configuration](Hypodermic::ComponentContext&,
                            const std::shared_ptr<ConnectionManager>& instance) {
                instance->initialize(
                    configuration["activemq"]["broker-url"].get<std::string>());
            })
        .singleInstance();

    // Producer de colas (inyectar como interfaz)
    builder.registerType<QueueMessageProducer>()
        .as<IQueueMessageProducer>()
        .singleInstance();

    // =========================
    // Repositorios
    // =========================

    // Torneos
    builder.registerType<TournamentRepository>()
        .as<IRepository<domain::Tournament, std::string>>()   // Tournament usa std::string
        .singleInstance();

    // Equipos
    builder.registerType<TeamRepository>()
        .as<IRepository<domain::Team, std::string_view>>()    // Team usa std::string_view
        .singleInstance();

    // Grupos
    builder.registerType<GroupRepository>()
        .as<IGroupRepository>()
        .singleInstance();

    // NUEVO: Matches
    builder.registerType<MatchRepository>()
        .as<IMatchRepository>()
        .singleInstance();

    // =========================
    // Delegates
    // =========================

    builder.registerType<TeamDelegate>()
        .as<ITeamDelegate>()
        .singleInstance();

    builder.registerType<TournamentDelegate>()
        .as<ITournamentDelegate>()
        .singleInstance();

    builder.registerType<GroupDelegate>()
        .as<IGroupDelegate>()
        .singleInstance();

    // NUEVO: MatchDelegate
    builder.registerType<MatchDelegate>()
        .as<IMatchDelegate>()
        .singleInstance();

    // =========================
    // Controllers
    // =========================

    builder.registerType<TeamController>().singleInstance();
    builder.registerType<TournamentController>().singleInstance();
    builder.registerType<GroupController>().singleInstance();

    // NUEVO: MatchController
    builder.registerType<MatchController>().singleInstance();

    // =========================
    // Build container
    // =========================
    return builder.build();
}

} // namespace config

#endif // RESTAPI_CONTAINER_SETUP_HPP
