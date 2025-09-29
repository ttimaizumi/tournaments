//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"

#include <utility>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view> > repository) : teamRepository(std::move(repository)) {
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    return teamRepository->ReadById(id.data());
}

std::string_view TeamDelegate::CreateTeam(const domain::Team& team){
    return teamRepository->Create(team);
}

std::string_view TeamDelegate::UpdateTeam(const domain::Team& team){
    return teamRepository->Update(team);
}