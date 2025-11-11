#ifndef B37FEB69_6E3C_4DA6_BBCA_1BD46BF5F632
#define B37FEB69_6E3C_4DA6_BBCA_1BD46BF5F632

#pragma once

#include <memory>
#include <regex>
#include <string>

#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/IMatchDelegate.hpp"

class MatchController
{
    std::shared_ptr<IMatchDelegate> delegate;
    static const std::regex kIdPattern;

public:
    explicit MatchController(const std::shared_ptr<IMatchDelegate>& d);

    crow::response GetMatches(const crow::request& req,
                              const std::string& tournamentId) const;

    crow::response GetMatch(const std::string& tournamentId,
                            const std::string& matchId) const;

    crow::response PatchScore(const crow::request& req,
                              const std::string& tournamentId,
                              const std::string& matchId) const;
};

#endif /* B37FEB69_6E3C_4DA6_BBCA_1BD46BF5F632 */
