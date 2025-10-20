//
// Created by root on 10/8/25.
//

#include "delegate/GroupDelegate.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include <utility>
#include <format>

GroupDelegate::GroupDelegate(
    const std::shared_ptr<IRepository<domain::Tournament, std::string>>& tournamentRepository,
    const std::shared_ptr<IGroupRepository>& groupRepository,
    const std::shared_ptr<IRepository<domain::Team, std::string_view>>& teamRepository,
    const std::shared_ptr<QueueMessageProducer>& producer
)
    : tournamentRepository(tournamentRepository), groupRepository(groupRepository), teamRepository(teamRepository), producer(producer){}

std::expected<std::string, std::string> GroupDelegate::CreateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // Validate max teams per group for the tournament format
    int maxTeamsPerGroup = tournament->Format().MaxTeamsPerGroup();
    if (!group.Teams().empty() && group.Teams().size() > maxTeamsPerGroup) {
        return std::unexpected(std::format("Group exceeds maximum teams allowed ({})", maxTeamsPerGroup));
    }

    domain::Group g = group;
    g.TournamentId() = tournament->Id();
    if (!g.Teams().empty()) {
        for (auto& t : g.Teams()) {
            auto team = teamRepository->ReadById(t.Id);
            if (team == nullptr) {
                return std::unexpected("Team doesn't exist");
            }
        }
    }
    auto id = groupRepository->Create(g);

    // Generate events for each team added to the group
    if (!g.Teams().empty()) {
        for (const auto& team : g.Teams()) {
            producer->SendMessage(std::format("{}:{}:{}", tournamentId, id, team.Id), "team.added.to.group");
        }
    }

    return id;
}

std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> GroupDelegate::GetGroups(const std::string_view& tournamentId) {
    try {
        return this->groupRepository->FindByTournamentId(tournamentId);
    } catch (const std::exception& e) {
        return std::unexpected("Error when reading to DB");
    }
}

std::expected<std::shared_ptr<domain::Group>, std::string> GroupDelegate::GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    try {
        auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
        return group;
    } catch (const std::exception& e) {
        return std::unexpected("Error when reading from DB");
    }
}

std::expected<void, std::string> GroupDelegate::UpdateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    auto existingGroup = groupRepository->FindByTournamentIdAndGroupId(tournamentId, group.Id());
    if (existingGroup == nullptr) {
        return std::unexpected("Group doesn't exist");
    }

    try {
        domain::Group g = group;
        g.TournamentId() = tournament->Id();
        groupRepository->Update(g);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Failed to update group: ") + e.what());
    }
}

std::expected<void, std::string> GroupDelegate::RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return std::unexpected("Group doesn't exist");
    }

    try {
        groupRepository->Delete(groupId.data());
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Failed to delete group: ") + e.what());
    }
}

std::expected<void, std::string> GroupDelegate::UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams) {
    const auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return std::unexpected("Group doesn't exist");
    }
    const auto tournament = tournamentRepository->ReadById(tournamentId.data());
    int maxTeams = tournament->Format().MaxTeamsPerGroup();
    if (group->Teams().size() + teams.size() > maxTeams) {
        return std::unexpected("Group at max capacity");
    }
    for (const auto& team : teams) {
        if (const auto groupTeams = groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id)) {
            return std::unexpected(std::format("Team {} already exist", team.Id));
        }
    }
    for (const auto& team : teams) {
        const auto persistedTeam = teamRepository->ReadById(team.Id);
        if (persistedTeam == nullptr) {
            return std::unexpected(std::format("Team {} doesn't exist", team.Id));
        }
        groupRepository->UpdateGroupAddTeam(groupId, persistedTeam);

        // Generate event for each team added to the group
        producer->SendMessage(std::format("{}:{}:{}", tournamentId, groupId, team.Id), "team.added.to.group");
    }
    return {};
}