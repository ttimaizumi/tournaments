//
// Created by tomas on 9/6/25.
//
#include <activemq/library/ActiveMQCPP.h>
#include <thread>

#include "configuration/ContainerSetup.hpp"
#include "cms/ConnectionManager.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();

    {
        std::println("before container");
        const auto container = config::containerSetup();
        std::println("after container");

        // Initialize ConnectionManager before starting threads to avoid race conditions
        std::println("Pre-initializing ConnectionManager...");
        auto connectionMgr = container->resolve<ConnectionManager>();
        std::println("ConnectionManager initialized");

        std::println("Creating team add thread...");
        std::thread teamAddThread([&] {
            std::println("Team add thread started");
            auto listener = container->resolve<GroupAddTeamListener>();
            std::println("Team add listener resolved");
            listener->Start("tournament.team-add");
            std::println("Team add thread exiting");
        });

        std::println("Creating score update thread...");
        std::thread scoreUpdateThread([&] {
            std::println("Score update thread started");
            auto listener = container->resolve<ScoreUpdateListener>();
            std::println("Score update listener resolved");
            listener->Start("tournament.score-update");
            std::println("Score update thread exiting");
        });

        std::println("Waiting for threads to complete...");
        teamAddThread.join();
        std::println("Team add thread joined");
        scoreUpdateThread.join();
        std::println("Score update thread joined");
        // while (true) {
        //     std::this_thread::sleep_for(std::chrono::seconds(5));
        // }
    }

    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}