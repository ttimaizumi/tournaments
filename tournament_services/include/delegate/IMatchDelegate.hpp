#ifndef IMATCH_DELEGATE_HPP
#define IMATCH_DELEGATE_HPP

#include <string_view>
#include <string>
#include <memory>
#include <vector>
#include <expected>

#include "domain/Match.hpp"

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;

    virtual std::expected<std::shared_ptr<domain::Match>, std::string>
        GetMatch(std::string_view tournamentId, std::string_view matchId) = 0;

    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        GetMatches(std::string_view tournamentId, std::string_view filter) = 0;

    virtual std::expected<void, std::string>
        UpdateMatchScore(std::string_view tournamentId, std::string_view matchId, const domain::Score& score) = 0;
};

#endif /* IMATCH_DELEGATE_HPP */
