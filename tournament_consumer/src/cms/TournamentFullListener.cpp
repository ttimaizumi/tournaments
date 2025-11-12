//
// Created by edgar on 11/12/25.
//

#include "cms/TournamentFullListener.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using nlohmann::json;

TournamentFullListener::TournamentFullListener(
const std::shared_ptr<ConnectionManager>& cm,
const std::shared_ptr<IMatchRepository>& matchRepository,
const std::shared_ptr<IGroupRepository>& groupRepository)
: QueueMessageListener(cm),
  matchRepo(matchRepository),
  groupRepo(groupRepository) {
    std::cout << "[TournamentFullListener] Initialized and ready to generate matches" << std::endl;
}
void TournamentFullListener::processMessage(const std::string& message) {
    std::cout << "[CONSUMER] TournamentFullListener received: " << message << std::endl;

    try {
        json evt = json::parse(message);

        if (!evt.contains("tournamentId") || !evt.contains("groupId")) {
            std::cout << "[ERROR] TournamentFullListener: Missing tournamentId or groupId" << std::endl;
            return;
        }

        std::string tournamentId = evt.at("tournamentId").get<std::string>();
        std::string groupId = evt.at("groupId").get<std::string>();

        std::cout << "[TournamentFullListener] Generating matches for tournament: " << tournamentId << std::endl;

        auto group = groupRepo->FindByTournamentIdAndGroupId(tournamentId, groupId);
        if (!group) {
            std::cout << "[ERROR] TournamentFullListener: Group not found for tournamentId=" << tournamentId << ", groupId=" << groupId << std::endl;
            return;
        }

        const auto& teams = group->Teams();
        std::cout << "[TournamentFullListener] Group has " << teams.size() << " teams" << std::endl;
        if (teams.size() != 32) {
            std::cout << "[ERROR] TournamentFullListener: Expected 32 teams, got " << teams.size() << std::endl;
            return;
        }

        std::cout << "[TournamentFullListener] Creating 16 initial matches (Winner Bracket Round 1)" << std::endl;

        for (size_t i = 0; i < 16; i++) {
            json matchDoc;
            matchDoc["tournamentId"] = tournamentId;
            matchDoc["groupId"] = groupId;
            matchDoc["round"] = 1;
            matchDoc["bracket"] = "W";
            matchDoc["status"] = "pending";
            matchDoc["homeTeamId"] = teams[i * 2].Id;
            matchDoc["homeTeamName"] = teams[i * 2].Name;
            matchDoc["visitorTeamId"] = teams[i * 2 + 1].Id;
            matchDoc["visitorTeamName"] = teams[i * 2 + 1].Name;
            matchDoc["score"] = json{{"home", 0}, {"visitor", 0}};

            auto matchId = matchRepo->Create(matchDoc);
            if (matchId.has_value()) {
                std::cout << "[TournamentFullListener] Created match " << (i+1) << "/16: "
                          << teams[i*2].Name << " vs " << teams[i*2+1].Name
                          << " (ID: " << matchId.value() << ")" << std::endl;
            } else {
                std::cout << "[ERROR] TournamentFullListener: Failed to create match " << (i+1)
          << " (repository returned empty optional)" << std::endl;

            }
        }

        std::cout << "[TournamentFullListener] ✓ Successfully created all 16 initial matches!" << std::endl;
    } catch (const std::exception &ex) {
        std::cout << "[ERROR] TournamentFullListener exception: " << ex.what() << std::endl;
    }
}

