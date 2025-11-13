//
// Created by tomas on 9/6/25.
//

#include <activemq/library/ActiveMQCPP.h>
#include <print>
#include <thread>
#include <chrono>

#include "configuration/ContainerSetup.hpp"
#include "cms/GroupAddTeamListener.hpp"
#include "cms/ScoreRecordedListener.hpp"
#include "cms/TournamentFullListener.hpp"

int main() {
    // Inicializa la librería de ActiveMQ
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::println("before container");
        auto container = config::containerSetup();
        std::println("after container");

        // Resolvemos listeners desde el contenedor de dependencias
        auto groupListener = container->resolve<GroupAddTeamListener>();
        auto scoreListener = container->resolve<ScoreRecordedListener>();
        auto fullListener  = container->resolve<TournamentFullListener>();

        // IMPORTANTE: nombres de cola que usa tu servicio
        //  - "group.team-added"  -> GroupDelegate::UpdateTeams
        //  - "match.score-recorded" -> MatchDelegate::UpdateScore
        //  - "tournament.full"   -> lo mandaremos cuando el torneo tenga 32 equipos
        groupListener->Start("group.team-added");
        scoreListener->Start("match.score-recorded");
        fullListener->Start("tournament.full");

        std::println("[consumer] listeners started. Waiting for messages...");

        // Mantener el proceso vivo "para siempre"
        // (lo paras con Stop en CLion o Ctrl+C en terminal)
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        // Normalmente no se llega aquí; al salir del bloque,
        // los destructores de los listeners llamarán Stop().
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}
