#include "delegate/GroupDelegate.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include <utility>
#include <format>

GroupDelegate::GroupDelegate(
    const std::shared_ptr<IRepository<domain::Tournament, std::string>>& tournamentRepository,
    const std::shared_ptr<IGroupRepository>& groupRepository,
    const std::shared_ptr<IRepository<domain::Team, std::string_view>>& teamRepository,
    const std::shared_ptr<QueueMessageProducer>& producer)
    : tournamentRepository(tournamentRepository),
      groupRepository(groupRepository),
      teamRepository(teamRepository),
      producer(producer) {}

std::expected<std::string, std::string>
GroupDelegate::CreateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr)
        return std::unexpected("Tournament doesn't exist");

    int maxTeamsPerGroup = tournament->Format().MaxTeamsPerGroup();
    if (!group.Teams().empty() && group.Teams().size() > maxTeamsPerGroup)
        return std::unexpected(std::format("Group exceeds maximum teams allowed ({})", maxTeamsPerGroup));

    domain::Group g = group;
    g.SetTournamentId(tournament->Id());

    for (const auto& t : g.Teams()) {
        if (teamRepository->ReadById(t.Id) == nullptr)
            return std::unexpected("Team doesn't exist");
    }

    try {
        auto id = groupRepository->Create(g);

        for (const auto& team : g.Teams())
            producer->SendMessage(std::format("{}:{}:{}", tournamentId, id, team.Id), "team.added.to.group");

        return id;
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}

std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
GroupDelegate::GetGroups(const std::string_view& tournamentId) {
    try {
        return this->groupRepository->FindByTournamentId(tournamentId);
    } catch (...) {
        return std::unexpected("Error when reading from DB");
    }
}

std::expected<std::shared_ptr<domain::Group>, std::string>
GroupDelegate::GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    try {
        return groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    } catch (...) {
        return std::unexpected("Error when reading from DB");
    }
}

std::expected<void, std::string>
GroupDelegate::UpdateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr)
        return std::unexpected("Tournament doesn't exist");

    auto existingGroup = groupRepository->FindByTournamentIdAndGroupId(tournamentId, group.Id());
    if (existingGroup == nullptr)
        return std::unexpected("Group doesn't exist");

    try {
        domain::Group g = group;
        g.SetTournamentId(tournament->Id());
        groupRepository->Update(g);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Failed to update group: ") + e.what());
    }
}

std::expected<void, std::string>
GroupDelegate::RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr)
        return std::unexpected("Tournament doesn't exist");

    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr)
        return std::unexpected("Group doesn't exist");

    try {
        groupRepository->Delete(groupId.data());
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Failed to delete group: ") + e.what());
    }
}

std::expected<void, std::string>
GroupDelegate::UpdateTeams(
    const std::string_view& tournamentId,
    const std::string_view& groupId,
    const std::vector<domain::Team>& teams) {
    const auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (tournament == nullptr)
        return std::unexpected("Tournament doesn't exist");

    const auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr)
        return std::unexpected("Group doesn't exist");

    int maxTeams = tournament->Format().MaxTeamsPerGroup();
    if (group->Teams().size() + teams.size() > maxTeams)
        return std::unexpected("Group at max capacity");

    for (const auto& team : teams) {
        if (auto existing = groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id))
            return std::unexpected(std::format("Team {} already exist", team.Id));
    }

    for (const auto& team : teams) {
        const auto persistedTeam = teamRepository->ReadById(team.Id);
        if (persistedTeam == nullptr)
            return std::unexpected(std::format("Team {} doesn't exist", team.Id));

        groupRepository->UpdateGroupAddTeam(groupId, persistedTeam);
        producer->SendMessage(std::format("{}:{}:{}", tournamentId, groupId, team.Id),
                              "team.added.to.group");
    }

    return {};
}
