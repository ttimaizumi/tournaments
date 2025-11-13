#ifndef MATCH_DELEGATE_HPP
#define MATCH_DELEGATE_HPP
#pragma once

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

#include "delegate/IMatchDelegate.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IMatchRepository> repository;
    std::shared_ptr<IQueueMessageProducer> producer;

    static bool isValidBracket(const std::string& b) {
        return (b == "winners" || b == "losers");
    }
    static bool isValidSlot(const std::string& s) {
        return (s == "home" || s == "visitor");
    }

public:
    MatchDelegate(std::shared_ptr<IMatchRepository> repo,
                  std::shared_ptr<IQueueMessageProducer> prod)
        : repository(std::move(repo)), producer(std::move(prod)) {}

    std::expected<nlohmann::json, std::string>
    CreateMatch(const std::string_view& tournamentId,
                const nlohmann::json& body) override;

    std::expected<std::vector<nlohmann::json>, std::string>
    GetMatches(const std::string_view& tournamentId,
               std::optional<std::string> statusFilter) override;

    std::expected<nlohmann::json, std::string>
    GetMatch(const std::string_view& tournamentId,
             const std::string_view& matchId) override;

    std::expected<void, std::string>
    UpdateScore(const std::string_view& tournamentId,
                const std::string_view& matchId,
                int homeScore,
                int visitorScore) override;
};

#endif // MATCH_DELEGATE_HPP
