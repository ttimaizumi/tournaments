#ifndef RESTAPI_MATCHDELEGATE_HPP
#define RESTAPI_MATCHDELEGATE_HPP

#include <memory>
#include <expected>
#include <string>

#include "persistence/repository/IMatchRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "domain/Match.hpp"
#include "IMatchDelegate.hpp"

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
    std::shared_ptr<IQueueMessageProducer> queueMessageProducer;

public:
    explicit MatchDelegate(std::shared_ptr<IMatchRepository> repository,
                          std::shared_ptr<IQueueMessageProducer> queueProducer);

    std::expected<std::shared_ptr<domain::Match>, std::string>
        GetMatch(std::string_view tournamentId, std::string_view matchId) override;

    std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        GetMatches(std::string_view tournamentId, std::string_view filter) override;

    std::expected<void, std::string>
        UpdateMatchScore(std::string_view tournamentId, std::string_view matchId, const domain::Score& score) override;
};

#endif //RESTAPI_MATCHDELEGATE_HPP
