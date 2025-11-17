//
// Created by developer on 10/14/25.
//

#ifndef LISTENER_GROUPADDTEAM_LISTENER_HPP
#define LISTENER_GROUPADDTEAM_LISTENER_HPP

#include "QueueMessageListener.hpp"

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <print>
#include <exception>

// Consumidor para el evento "group.team-added".

class GroupAddTeamListener : public QueueMessageListener {
public:
    GroupAddTeamListener(const std::shared_ptr<ConnectionManager>& connectionManager);
    ~GroupAddTeamListener() override;

protected:
    void processMessage(const std::string& message) override;
};

inline GroupAddTeamListener::GroupAddTeamListener(
        const std::shared_ptr<ConnectionManager>& connectionManager)
    : QueueMessageListener(connectionManager) {
}

inline GroupAddTeamListener::~GroupAddTeamListener() {
    Stop();
}

inline void GroupAddTeamListener::processMessage(const std::string& message) {
    using nlohmann::json;
    std::println("[consumer] group.team-added recibido: {}", message);

    try {
        auto payload = json::parse(message);

        std::string tournamentId = payload.value("tournamentId", "");
        std::string groupId      = payload.value("groupId", "");
        std::string teamId       = payload.value("teamId", "");
        std::string teamName     = payload.value("teamName", "");

        if (tournamentId.empty() || groupId.empty() || teamId.empty()) {
            std::println("[consumer] mensaje group.team-added incompleto, se ignora.");
            return;
        }

        struct TeamInfo {
            std::string id;
            std::string name;
        };

        // Mapa en memoria: clave = torneo:grupo, valor = lista de equipos agregados
        static std::unordered_map<std::string, std::vector<TeamInfo>> pendingTeams;

        std::string key = tournamentId + ":" + groupId;
        auto& bucket = pendingTeams[key];
        bucket.push_back(TeamInfo{teamId, teamName});

        std::println("[consumer] ahora hay {} equipos registrados en torneo={} grupo={}",
                     bucket.size(), tournamentId, groupId);

        // Logica : cada vez que se completan pares de equipos, "creamos" un partido
        if (bucket.size() % 2 == 0) {
            const auto& home    = bucket[bucket.size() - 2];
            const auto& visitor = bucket[bucket.size() - 1];

            std::println("[consumer] (demo) aqui se crearia partido de fase de grupos {} vs {}",
                         home.name, visitor.name);
        }


    } catch (const std::exception& e) {
        std::println("[consumer] error al procesar mensaje group.team-added: {}", e.what());
    }
}

#endif //LISTENER_GROUPADDTEAM_LISTENER_HPP
