//
// Created by tomas on 9/6/25.
//
#include <activemq/library/ActiveMQCPP.h>
#include <thread>

#include "configuration/ContainerSetup.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::println("before container");
        const auto container = config::containerSetup();
        std::println("after container");

        std::thread teamAddThread([&] {
            auto listener = container->resolve<GroupAddTeamListener>();
            listener->Start("tournament.team-add");
        });
        
        std::thread scoreUpdateThread([&] {
            auto listener = container->resolve<MatchScoreUpdateListener>();
            listener->Start("tournament.score-update");
        });

        teamAddThread.join();
        scoreUpdateThread.join();
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}