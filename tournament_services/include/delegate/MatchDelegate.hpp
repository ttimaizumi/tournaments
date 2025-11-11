#ifndef RESTAPI_MATCH_DELEGATE_HPP
#define RESTAPI_MATCH_DELEGATE_HPP

#include <expected>
#include <string>
#include <vector>
#include <memory>
#include "domain/Match.hpp"
#include "exception/Error.hpp"
#include "delegate/IMatchDelegate.hpp"
#include "persistence/repository/IMatchRepository.hpp"

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
public:
    explicit MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository);
    std::expected<std::shared_ptr<domain::Match>, Error> GetMatch(std::string_view tournamentId, std::string_view matchId) override;
    std::expected<std::vector<std::shared_ptr<domain::Match>>, Error> GetMatches(std::string_view tournamentId) override;
    std::expected<std::string, Error> UpdateMatchScore(const domain::Match& match) override;
};  

#endif /* RESTAPI_MATCH_DELEGATE_HPP */