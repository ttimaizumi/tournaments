//
// Created by tomas on 9/6/25.
//
#include <activemq/library/ActiveMQCPP.h>
#include <thread>
#include <print>
#include "configuration/ContainerSetup.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::println("before container");
        const auto container = config::containerSetup();
        std::println("after container");

        // Listener ya existente: ejemplo de grupo
        std::thread groupThread([&] {
            auto listener = container->resolve<GroupAddTeamListener>();
            listener->Start("tournament.team-add");
        });

        // Nuevo listener: score de match
        std::thread scoreThread([&] {
            auto listener = container->resolve<ScoreRecordedListener>();
            listener->Start("match.score-recorded");
        });

        groupThread.join();
        scoreThread.join();
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}
