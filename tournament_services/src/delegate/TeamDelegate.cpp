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
    return teamRepository->ReadById(id.data());
}

std::string_view TeamDelegate::SaveTeam(const domain::Team& team){
    auto teamRepo = dynamic_cast<TeamRepository*>(teamRepository.get());

    if(teamRepo && teamRepo->ExistsByName(team.Name)) {
        return "";
    }

    return teamRepository->Create(team);
}

std::string_view TeamDelegate::UpdateTeam(const domain::Team& team){
    auto teamRepo = dynamic_cast<TeamRepository*>(teamRepository.get());
    if(teamRepo && !teamRepo->ExistsById(team.Id)) {
        return "";
    }

    return teamRepository->Update(team);
}


