#ifndef SUPPORT_TEAMCONTROLLER_HPP
#define SUPPORT_TEAMCONTROLLER_HPP

#include <utility>
#include <optional>
#include <vector>
#include <string>
#include "model/Team.hpp"
#include "delegate/ITeamDelegate.hpp"

// Stub de TeamController para tests
class TeamController {
public:
    explicit TeamController(ITeamDelegate* delegate) : delegate_(delegate) {}

    int PostTeam(const Team& team, std::string* location) {
        auto result = delegate_->Create(team);
        if (result) {
            if (location) {
                *location = *result;
            }
            return 201;
        }
        return result.error();
    }

    std::pair<int, std::optional<Team>> GetTeam(const std::string& id) {
        auto result = delegate_->FindById(id);
        if (result) {
            return {200, *result};
        }
        return {404, std::nullopt};
    }

    std::pair<int, std::vector<Team>> GetTeams() {
        return {200, delegate_->List()};
    }

    int PatchTeam(const std::string& id, const Team& team) {
        auto result = delegate_->Update(id, team);
        if (result) {
            return 204;
        }
        return result.error();
    }

private:
    ITeamDelegate* delegate_;
};

#endif
