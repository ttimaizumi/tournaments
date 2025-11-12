#include "delegate/GroupDelegate.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using nlohmann::json;

static constexpr std::size_t kMaxTeamsPerGroup = 32;
GroupDelegate::GroupDelegate(
    const std::shared_ptr<IRepository<domain::Tournament, std::string>>& tRepo,
    const std::shared_ptr<IGroupRepository>& gRepo,
    const std::shared_ptr<IRepository<domain::Team, std::string_view>>& teamRepo, // <- std::string
    const std::shared_ptr<IQueueMessageProducer>& producer)
    : tournamentRepository(tRepo)
    , groupRepository(gRepo)
    , teamRepository(teamRepo)
    , queueProducer(producer)
{}

std::expected<std::string, std::string>
GroupDelegate::CreateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    if (!tournamentRepository || !groupRepository || !teamRepository) {
        return std::unexpected(std::string("misconfigured-dependencies"));
    }

    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) return std::unexpected(std::string("tournament-not-found"));

    domain::Group g = group;
    g.TournamentId() = tournament->Id();

    // valida equipos (si vienen en el body)
    for (const auto& t : g.Teams()) {
        auto team = teamRepository->ReadById(std::string{t.Id});
        if (!team) return std::unexpected(std::string("team-not-found"));
    }

    auto id = groupRepository->Create(g);
    if (id.empty()) return std::unexpected(std::string("group-already-exists"));

    if (queueProducer) {
        // queueProducer->SendMessage(id, "group.created"); // test espera ("g1","group.created")
        queueProducer->SendMessage(id, "group.created");
    }
    return id;
}

std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
GroupDelegate::GetGroups(const std::string_view& tournamentId) {
    try {
        return groupRepository->FindByTournamentId(tournamentId);
    } catch (...) {
        return std::unexpected(std::string("db-error: groups"));
    }
}

std::expected<std::shared_ptr<domain::Group>, std::string>
GroupDelegate::GetGroup(const std::string_view& tournamentId,
                        const std::string_view& groupId) {
    try {
        auto g = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
        if (!g) return std::unexpected(std::string("group-not-found")); // el test lo espera así
        return g;
    } catch (...) {
        return std::unexpected(std::string("db-error: group"));
    }
}

std::expected<void, std::string>
GroupDelegate::UpdateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    if (!groupRepository) return std::unexpected(std::string("misconfigured-dependencies"));
    auto existing = groupRepository->FindByTournamentIdAndGroupId(tournamentId, group.Id());
    if (!existing) return std::unexpected(std::string("group-not-found"));

    domain::Group g = group;
    g.TournamentId() = std::string(tournamentId);

    auto id = groupRepository->Update(g);
    if (id.empty()) return std::unexpected(std::string("db-error: update-group"));
    return {};
}

std::expected<void, std::string>
GroupDelegate::RemoveGroup(const std::string_view& tournamentId,
                           const std::string_view& groupId) {
    if (!groupRepository) return std::unexpected(std::string("misconfigured-dependencies"));
    auto existing = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!existing) return std::unexpected(std::string("group-not-found"));
    groupRepository->Delete(std::string(groupId));
    return {};
}

std::expected<void, std::string>
GroupDelegate::UpdateTeams(const std::string_view& tournamentId,
                           const std::string_view& groupId,
                           const std::vector<domain::Team>& teams) {
    if (!groupRepository || !teamRepository) return std::unexpected(std::string("misconfigured-dependencies"));

    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) return std::unexpected(std::string("group-not-found"));

    if (group->Teams().size() + teams.size() > kMaxTeamsPerGroup)
        return std::unexpected(std::string("group-full"));

    // 1) validar que ninguno ya esté en otro grupo del torneo
    for (const auto& t : teams) {
        auto located = groupRepository->FindByTournamentIdAndTeamId(tournamentId, t.Id);
        if (located) return std::unexpected(std::string("team-already-in-group"));
    }

    // 2) pre-validar existencia de cada team (primer ReadById)
    for (const auto& t : teams) {
        auto exists = teamRepository->ReadById(std::string{t.Id});
        if (!exists) return std::unexpected(std::string("team-not-found"));
    }

    // 3) añadir y publicar evento (segundo ReadById)
    for (const auto& t : teams) {
        auto persistedTeam = teamRepository->ReadById(std::string{t.Id});
        if (!persistedTeam) return std::unexpected(std::string("team-not-found"));

        groupRepository->UpdateGroupAddTeam(std::string(groupId), persistedTeam);

        if (queueProducer) {
            // Enviar evento con información completa
            nlohmann::json eventPayload;
            eventPayload["tournamentId"] = std::string(tournamentId);
            eventPayload["groupId"] = std::string(groupId);
            eventPayload["teamId"] = persistedTeam->Id;
            eventPayload["teamName"] = persistedTeam->Name;

            std::string messageBody = eventPayload.dump();
            queueProducer->SendMessage(messageBody, "group.team-added");
        }
    }

    // 4) Verificar si se completaron los 32 equipos para generar matches
    auto updatedGroup = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (updatedGroup && updatedGroup->Teams().size() == 32 && queueProducer) {
        nlohmann::json tournamentFullEvent;
        tournamentFullEvent["tournamentId"] = std::string(tournamentId);
        tournamentFullEvent["groupId"] = std::string(groupId);
        tournamentFullEvent["teamCount"] = updatedGroup->Teams().size();

        std::string messageBody = tournamentFullEvent.dump();
        queueProducer->SendMessage(messageBody, "tournament.full");

        std::cout << "[GroupDelegate] Tournament FULL! 32 teams reached. Event sent to generate matches." << std::endl;
    }
    return {};
}
