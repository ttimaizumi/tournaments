#ifndef SUPPORT_TOURNAMENTCONTROLLER_HPP
#define SUPPORT_TOURNAMENTCONTROLLER_HPP

#include <utility>
#include <optional>
#include <vector>
#include <string>
#include "model/Tournament.hpp"
#include "delegate/ITournamentDelegate.hpp"

// Stub de TournamentController para tests
class TournamentController {
public:
    explicit TournamentController(ITournamentDelegate* delegate) : delegate_(delegate) {}

    int PostTournament(const Tournament& tournament, std::string* location) {
        auto result = delegate_->Create(tournament);
        if (result) {
            if (location) {
                *location = *result;
            }
            return 201;
        }
        return result.error();
    }

    std::pair<int, std::optional<Tournament>> GetTournament(const std::string& id) {
        auto result = delegate_->FindById(id);
        if (result) {
            return {200, *result};
        }
        return {404, std::nullopt};
    }

    std::pair<int, std::vector<Tournament>> GetTournaments() {
        return {200, delegate_->List()};
    }

    int PatchTournament(const std::string& id, const Tournament& tournament) {
        auto result = delegate_->Update(id, tournament);
        if (result) {
            return 204;
        }
        return result.error();
    }

private:
    ITournamentDelegate* delegate_;
};

#endif
