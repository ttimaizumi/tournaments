#ifndef RESTAPI_IMATCH_DELEGATE_HPP
#define RESTAPI_IMATCH_DELEGATE_HPP

#include <string_view>
#include <memory>
#include <vector>
#include <string>
#include <expected>

#include "domain/Match.hpp"
#include "exception/Error.hpp"

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;
    virtual std::expected<std::shared_ptr<domain::Match>, Error> GetMatch(std::string_view tournamentId, std::string_view matchId) = 0;
    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, Error> GetMatches(std::string_view tournamentId) = 0;
    virtual std::expected<std::string, Error> UpdateMatchScore(const domain::Match& match) = 0;
};
#endif /* RESTAPI_IMATCH_DELEGATE_HPP */