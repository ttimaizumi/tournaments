#include "delegate/GroupDelegate.hpp"
#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"
#include "exception/Duplicate.hpp"

#include <utility>
#include <sstream>

GroupDelegate::GroupDelegate(const std::shared_ptr<TournamentRepository>& tournamentRepository, const std::shared_ptr<IGroupRepository>& groupRepository, const std::shared_ptr<TeamRepository>& teamRepository)
    : tournamentRepository(tournamentRepository), groupRepository(groupRepository), teamRepository(teamRepository){}

std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> GroupDelegate::GetGroups(const std::string_view& tournamentId) {
    try {
        tournamentRepository->ReadById(tournamentId.data());
        return this->groupRepository->FindByTournamentId(tournamentId);
    } catch (const NotFoundException& e) {
        throw; 
    } catch (const InvalidFormatException& e) {
        throw;
    } catch (const std::exception& e) {
        return std::unexpected("Error when reading to DB.");
    }
}

std::expected<std::string, std::string> GroupDelegate::CreateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    try {
        tournamentRepository->ReadById(tournamentId.data());
        domain::Group g = group;
        g.TournamentId() = tournamentId.data();
        if (!g.Teams().empty()) {
            for (auto& t : g.Teams()) {
                teamRepository->ReadById(t.Id);
            }
        }
        return this->groupRepository->Create(g);
    } catch (const NotFoundException& e) {
        throw;
    } catch (const InvalidFormatException& e) {
        throw;
    } catch (const DuplicateException& e) {
        throw;
    } catch (const std::exception& e) {
        return std::unexpected("Error creating group: " + std::string(e.what()));
    }
}

std::expected<std::shared_ptr<domain::Group>, std::string> GroupDelegate::GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    try {
        return groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    } catch (const std::exception& e) {
        return std::unexpected("Error when reading to DB");
    }
}
std::expected<void, std::string> GroupDelegate::UpdateGroup(const std::string_view& tournamentId, const domain::Group& group, const std::string_view& groupId) {
    try {
        tournamentRepository->ReadById(tournamentId.data());
        groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
        
        domain::Group updatedGroup = group;
        updatedGroup.Id() = groupId;
        updatedGroup.TournamentId() = tournamentId;
        
        groupRepository->Update(updatedGroup);
        
        return {};
    } catch (const NotFoundException& e) {
        throw;
    } catch (const InvalidFormatException& e) {
        throw;
    } catch (const std::exception& e) {
        return std::unexpected("Error updating group: " + std::string(e.what()));
    }
}
std::expected<void, std::string> GroupDelegate::RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    //Falta implementar
    return std::unexpected("Not implemented");
}

std::expected<void, std::string> GroupDelegate::UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams) {
    const auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return std::unexpected("Group doesn't exist");
    }
    if (group->Teams().size() + teams.size() >= 16) {
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
    }
    return {};
}
