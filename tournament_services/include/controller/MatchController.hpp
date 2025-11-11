#ifndef RESTAPI_MATCH_CONTROLLER_HPP
#define RESTAPI_MATCH_CONTROLLER_HPP

#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <regex>

#include "delegate/IMatchDelegate.hpp"
#include "domain/Constants.hpp"

class MatchController {
    std::shared_ptr<IMatchDelegate> matchDelegate;
public:
    explicit MatchController(const std::shared_ptr<IMatchDelegate>& matchDelegate);
    crow::response getMatch(const std::string& tournamentId, const std::string& matchId);
    crow::response getMatches(const std::string& tournamentId);
    crow::response updateMatchScore(const crow::request& request, const std::string& tournamentId, const std::string& matchId);
};


#endif /* RESTAPI_MATCH_CONTROLLER_HPP */