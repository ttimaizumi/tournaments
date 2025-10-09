//
// TeamDelegate.cpp
//

#include "delegate/TeamDelegate.hpp"

#include <expected>   // para std::expected
#include <utility>
#include <string>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository)
    : teamRepository(std::move(repository)) {}


std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    return teamRepository->ReadById(id);
}

std::string_view TeamDelegate::SaveTeam(const domain::Team& team) {
    return teamRepository->Create(team);
}

bool TeamDelegate::UpdateTeam(const domain::Team& team) {
    auto existing = teamRepository->ReadById(team.Id);
    if (!existing) return false;                 // no encontrado
    auto updatedId = teamRepository->Update(team);
    return !updatedId.empty();                   // true si update retorno id no vacio
}

/*** nuevas variantes con std::expected ***/
std::expected<std::string, std::string>
TeamDelegate::SaveTeamEx(const domain::Team& team) {
    std::string_view id = teamRepository->Create(team);
    if (id.empty()) {
        return std::unexpected(std::string{"conflict"});
    }
    return std::string{id};
}

std::expected<void, std::string>
TeamDelegate::UpdateTeamEx(const domain::Team& team) {
    auto existing = teamRepository->ReadById(team.Id);
    if (!existing) {
        return std::unexpected(std::string{"not found"});
    }
    std::string_view updatedId = teamRepository->Update(team);
    if (updatedId.empty()) {
        return std::unexpected(std::string{"conflict"});
    }
    return {}; // ok
}
