#ifndef SUPPORT_MATCHCONTROLLER_HPP
#define SUPPORT_MATCHCONTROLLER_HPP

#include <utility>
#include <optional>
#include <vector>
#include <string>
#include "domain/Match.hpp"
#include "delegate/IMatchDelegate.hpp"

// Stub de MatchController para tests
class MatchController {
public:
    explicit MatchController(IMatchDelegate* delegate) : delegate_(delegate) {}

    int PostMatch(const std::string& tournamentId, const Match& match, std::string* location) {
        auto result = delegate_->CreateMatch(tournamentId, match);
        if (result) {
            if (location) {
                *location = *result;
            }
            return 201;
        }
        return result.error();
    }

    std::pair<int, std::optional<Match>> GetMatch(const std::string& matchId) {
        auto result = delegate_->FindMatchById(matchId);
        if (result) {
            return {200, *result};
        }
        return {404, std::nullopt};
    }

    std::pair<int, std::vector<Match>> GetMatchesByGroup(const std::string& tournamentId, const std::string& groupId) {
        return {200, delegate_->ListMatchesByGroup(tournamentId, groupId)};
    }

    int PatchScore(const std::string& matchId, int homeScore, int awayScore) {
        auto result = delegate_->UpdateScore(matchId, homeScore, awayScore);
        if (result) {
            return 200;
        }
        return result.error();
    }

    int PostFinishMatch(const std::string& matchId) {
        auto result = delegate_->FinishMatch(matchId);
        if (result) {
            return 204;
        }
        return result.error();
    }

private:
    IMatchDelegate* delegate_;
};

#endif
