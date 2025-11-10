#ifndef RESTAPI_MATCH_CONTROLLER_HPP
#define RESTAPI_MATCH_CONTROLLER_HPP

#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>

#include "delegate/IMatchDelegate.hpp"

class MatchController {
    std::shared_ptr<IMatchDelegate> matchDelegate;
public:
    explicit MatchController(const std::shared_ptr<IMatchDelegate>& matchDelegate);

    [[nodiscard]] crow::response getMatches(const crow::request& request, const std::string& tournamentId) const;
    [[nodiscard]] crow::response getMatch(const std::string& tournamentId, const std::string& matchId) const;
    [[nodiscard]] crow::response updateMatchScore(const crow::request& request, const std::string& tournamentId, const std::string& matchId) const;
};

#endif //RESTAPI_MATCH_CONTROLLER_HPP
