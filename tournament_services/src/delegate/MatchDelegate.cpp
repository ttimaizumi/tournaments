#include "delegate/MatchDelegate.hpp"
#include <utility>
#include <nlohmann/json.hpp>

MatchDelegate::MatchDelegate(std::shared_ptr<IMatchRepository> repository,
                             std::shared_ptr<IQueueMessageProducer> queueProducer)
    : matchRepository(std::move(repository)), queueMessageProducer(std::move(queueProducer)) {
}

std::expected<std::shared_ptr<domain::Match>, std::string>
MatchDelegate::GetMatch(std::string_view tournamentId, std::string_view matchId) {
    try {
        auto match = matchRepository->FindByTournamentIdAndMatchId(tournamentId, matchId);
        if (match == nullptr) {
            return std::unexpected("Match not found");
        }
        return match;
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error reading match: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error reading match");
    }
}

std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
MatchDelegate::GetMatches(std::string_view tournamentId, std::string_view filter) {
    try {
        // Check if tournament exists
        if (!matchRepository->TournamentExists(tournamentId)) {
            return std::unexpected("Tournament not found");
        }

        if (filter == "played") {
            return matchRepository->FindPlayedByTournamentId(tournamentId);
        } else if (filter == "pending") {
            return matchRepository->FindPendingByTournamentId(tournamentId);
        } else {
            return matchRepository->FindByTournamentId(tournamentId);
        }
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error reading matches: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error reading matches");
    }
}

std::expected<void, std::string>
MatchDelegate::UpdateMatchScore(std::string_view tournamentId, std::string_view matchId, const domain::Score& score) {
    try {
        // Get the match first
        auto match = matchRepository->FindByTournamentIdAndMatchId(tournamentId, matchId);
        if (match == nullptr) {
            return std::unexpected("Match not found");
        }

        // Validate score is non-negative
        if (score.homeTeamScore < 0 || score.visitorTeamScore < 0) {
            return std::unexpected("Score values must be non-negative");
        }

        // Validate no ties in playoff rounds
        if (match->GetRound() != domain::Round::REGULAR) {
            if (score.IsTie()) {
                return std::unexpected("Tie not allowed in playoff matches");
            }
        }

        // Update the match with the new score
        match->SetScore(score);
        matchRepository->Update(*match);

        // Publish event to ActiveMQ
        nlohmann::json eventJson = {
            {"tournamentId", std::string(tournamentId)},
            {"matchId", std::string(matchId)},
            {"score", {
                {"home", score.homeTeamScore},
                {"visitor", score.visitorTeamScore}
            }}
        };
        queueMessageProducer->SendMessage(eventJson.dump(), "tournament.score-update");

        return {};
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error updating match score: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error updating match score");
    }
}
