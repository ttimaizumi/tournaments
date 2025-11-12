#ifndef MATCH_CONTROLLER_HPP
#define MATCH_CONTROLLER_HPP
#pragma once

#include <memory>
#include <regex>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>
#include "delegate/IMatchDelegate.hpp"

class MatchController {
    std::shared_ptr<IMatchDelegate> delegate;

public:
    static const std::regex kIdPattern;

    explicit MatchController(const std::shared_ptr<IMatchDelegate>& d)
        : delegate(d) {}

    // POST /tournaments/<id>/matches
    crow::response CreateMatch(const crow::request& req,
                               const std::string& tournamentId) const;

    // GET /tournaments/<id>/matches?status=played|scheduled
    crow::response GetMatches(const crow::request& req,
                              const std::string& tournamentId) const;

    // GET /tournaments/<id>/matches/<matchId>
    crow::response GetMatch(const std::string& tournamentId,
                            const std::string& matchId) const;

    // PATCH /tournaments/<id>/matches/<matchId>  body: {"score":{"home":X,"visitor":Y}}
    crow::response PatchScore(const crow::request& req,
                              const std::string& tournamentId,
                              const std::string& matchId) const;
};

#endif // MATCH_CONTROLLER_HPP
