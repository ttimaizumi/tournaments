#include "delegate/MatchDelegate.hpp"

#include <expected>
#include <iostream>
#include <regex>
#include <utility>
#include <pqxx/pqxx>

#include "exception/Error.hpp"
#include "domain/Constants.hpp"

MatchDelegate::MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository)
    : matchRepository(matchRepository) {}

std::expected<std::vector<std::shared_ptr<domain::Match>>, Error> MatchDelegate::GetMatches(std::string_view tournamentId) {
    if (!std::regex_match(std::string{tournamentId}, ID_VALUE)) {
        return std::unexpected(Error::INVALID_FORMAT);
    }
    std::cout << "Attempting to get matches for tournament ID: " + std::string(tournamentId) + "\n" ;
    return matchRepository->FindByTournamentId(tournamentId);
}

std::expected<std::shared_ptr<domain::Match>, Error> MatchDelegate::GetMatch(std::string_view tournamentId, std::string_view matchId) {
    if (!std::regex_match(std::string{tournamentId}, ID_VALUE) || 
        !std::regex_match(std::string{matchId}, ID_VALUE)) {
        return std::unexpected(Error::INVALID_FORMAT);
    }
    return matchRepository->FindByTournamentIdAndMatchId(tournamentId, matchId);
}

std::expected<std::string, Error> MatchDelegate::UpdateMatchScore(const domain::Match& match) {
    return "";
}