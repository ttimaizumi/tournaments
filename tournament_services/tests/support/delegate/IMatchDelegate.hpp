#ifndef SUPPORT_IMATCHDELEGATE_HPP
#define SUPPORT_IMATCHDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "domain/Match.hpp"

// Stub de interfaz para tests - IMatchDelegate
class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;
    virtual std::expected<std::string, int> CreateMatch(const std::string& tournamentId, const Match& match) = 0;
    virtual std::optional<Match> FindMatchById(const std::string& matchId) = 0;
    virtual std::vector<Match> ListMatchesByGroup(const std::string& tournamentId, const std::string& groupId) = 0;
    virtual std::expected<void, int> UpdateScore(const std::string& matchId, int homeScore, int awayScore) = 0;
    virtual std::expected<void, int> FinishMatch(const std::string& matchId) = 0;
};

#endif // SUPPORT_IMATCHDELEGATE_HPP
