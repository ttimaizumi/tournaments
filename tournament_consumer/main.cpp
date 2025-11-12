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

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::println("before container");
        auto container = config::containerSetup();
        std::println("after container");

        // Resolvemos listeners desde el contenedor de dependencias
        auto groupListener  = container->resolve<GroupAddTeamListener>();
        auto scoreListener  = container->resolve<ScoreRecordedListener>();

        // IMPORTANTE: aquí usamos los nombres de cola
        // que sí está usando tu servicio:
        //  - "group.team-added" lo manda GroupDelegate::UpdateTeams
        //  - "match.score-recorded" lo manda MatchDelegate::UpdateScore
        groupListener->Start("group.team-added");
        scoreListener->Start("match.score-recorded");

        std::println("[consumer] listeners started. Waiting for messages...");

        // Mantener el proceso vivo "para siempre"
        // (lo paras con Stop en CLion o Ctrl+C en terminal)
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        // NOTA: normalmente no se llega aquí; al salir del bloque,
        // los destructores de los listeners llamarán Stop().
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}