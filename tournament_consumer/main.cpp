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

        auto teamAddListener = container->resolve<GroupAddTeamListener>();
        auto scoreUpdateListener = container->resolve<MatchScoreUpdateListener>();

        std::println("Starting listeners...");

        std::thread teamAddThread([teamAddListener] {
            std::println("[Main] Starting team-add listener thread");
            teamAddListener->Start("tournament.team-add");
        });
        
        std::thread scoreUpdateThread([scoreUpdateListener] {
            std::println("[Main] Starting score-update listener thread");
            scoreUpdateListener->Start("tournament.score-update");
        });

        // Give threads time to start!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::println("Listeners started, press Ctrl+C to exit...");
        
        teamAddThread.join();
        scoreUpdateThread.join();
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}