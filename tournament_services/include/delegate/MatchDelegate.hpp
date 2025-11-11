#ifndef A251C297_DF53_4BEB_93D6_DB45EAC8C825
#define A251C297_DF53_4BEB_93D6_DB45EAC8C825

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
#include "cms/IQueueMessageProducer.hpp"   // para eventos de score, igual que otros delegates
#include "domain/Utilities.hpp"           // Score, Match, etc.

class MatchDelegate : public IMatchDelegate
{
    std::shared_ptr<IMatchRepository> repository;
    std::shared_ptr<IQueueMessageProducer> producer;

    static bool IsSingleElimination(const nlohmann::json& matchDoc);

public:
    MatchDelegate(std::shared_ptr<IMatchRepository> repo,
                  std::shared_ptr<IQueueMessageProducer> prod);

    std::expected<std::vector<nlohmann::json>, std::string>
    GetMatches(const std::string_view& tournamentId,
               std::optional<std::string> showMatches) override;

    std::expected<nlohmann::json, std::string>
    GetMatch(const std::string_view& tournamentId,
             const std::string_view& matchId) override;

    std::expected<void, std::string>
    UpdateScore(const std::string_view& tournamentId,
                const std::string_view& matchId,
                int homeScore,
                int visitorScore) override;
};


#endif /* A251C297_DF53_4BEB_93D6_DB45EAC8C825 */
