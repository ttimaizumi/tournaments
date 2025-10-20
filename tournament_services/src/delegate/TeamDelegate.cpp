//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"
#include "persistence/repository/TeamRepository.hpp"

#include <utility>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view> > repository) : teamRepository(std::move(repository)) {
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    try {
        return teamRepository->ReadById(id.data());
    } catch(...) {
        return nullptr;
    }
}

std::expected<std::string, std::string> TeamDelegate::SaveTeam(const domain::Team& team){
    auto teamRepo = dynamic_cast<TeamRepository*>(teamRepository.get());

    if(teamRepo && teamRepo->ExistsByName(team.Name)) {
        return std::unexpected("Team already exists");
    }

    try {
        std::string id(teamRepository->Create(team));
        return id;
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Failed to create team: ") + e.what());
    }
}

std::expected<std::string, std::string> TeamDelegate::UpdateTeam(const domain::Team& team){
    auto teamRepo = dynamic_cast<TeamRepository*>(teamRepository.get());
    if(teamRepo && !teamRepo->ExistsById(team.Id)) {
        return std::unexpected("Team not found");
    }

    try {
        std::string id(teamRepository->Update(team));
        return id;
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Failed to update team: ") + e.what());
    }
}


