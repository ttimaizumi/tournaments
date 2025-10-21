//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"
#include "persistence/repository/TeamRepository.hpp"

#include <utility>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view> > repository) : teamRepository(std::move(repository)) {
}

std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string> TeamDelegate::GetAllTeams() {
    try {
        return teamRepository->ReadAll();
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error reading teams: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error reading teams");
    }
}

std::expected<std::shared_ptr<domain::Team>, std::string> TeamDelegate::GetTeam(std::string_view id) {
    try {
        auto team = teamRepository->ReadById(id.data());
        if (team == nullptr) {
            return nullptr;
        }
        return team;
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error reading team: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error reading team");
    }
}

std::expected<std::string, std::string> TeamDelegate::SaveTeam(const domain::Team& team){
    auto teamRepo = dynamic_cast<TeamRepository*>(teamRepository.get());

    if(teamRepo && teamRepo->ExistsByName(team.Name)) {
        return std::unexpected("Team already exists");
    }

    try {
        std::string_view id = teamRepository->Create(team);
        return std::string(id);
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error creating team: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error creating team");
    }
}

std::expected<std::string, std::string> TeamDelegate::UpdateTeam(const domain::Team& team){
    auto teamRepo = dynamic_cast<TeamRepository*>(teamRepository.get());
    if(teamRepo && !teamRepo->ExistsById(team.Id)) {
        return std::unexpected("Team not found");
    }

    try {
        std::string_view id = teamRepository->Update(team);
        return std::string(id);
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error updating team: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error updating team");
    }
}

