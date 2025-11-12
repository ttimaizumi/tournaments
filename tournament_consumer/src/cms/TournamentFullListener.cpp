//
// Created by edgar on 11/12/25.
//

#include "cms/TournamentFullListener.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using nlohmann::json;

TournamentFullListener::TournamentFullListener(
    const std::shared_ptr<ConnectionManager>& cm)
    : QueueMessageListener(cm) {}

void TournamentFullListener::processMessage(const std::string& message) {
    try {
        json evt = json::parse(message);

        if (evt.contains("tournamentId")) {
            std::string tournamentId = evt.at("tournamentId").get<std::string>();
            std::cout << "[CONSUMER] tournament.full recibido para torneo: "
                      << tournamentId << std::endl;
        } else {
            std::cout << "[CONSUMER] tournament.full recibido (payload sin tournamentId): "
                      << message << std::endl;
        }
    } catch (const std::exception &ex) {
        std::cout << "[ERROR] TournamentFullListener: " << ex.what() << std::endl;
    }
}
