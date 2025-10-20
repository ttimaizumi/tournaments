#ifndef ITEAM_DELEGATE_HPP
#define ITEAM_DELEGATE_HPP

#include <string_view>
#include <memory>
#include <vector>
#include <string>
#include <expected>

#include "domain/Team.hpp"
#include "exception/Error.hpp"

class ITeamDelegate {
public:
    virtual ~ITeamDelegate() = default;

    virtual std::expected<std::shared_ptr<domain::Team>, Error> GetTeam(std::string_view id) = 0;
    virtual std::expected<std::vector<std::shared_ptr<domain::Team>>, Error> GetAllTeams() = 0;
    virtual std::expected<std::string, Error> CreateTeam(const domain::Team& team) = 0;
    virtual std::expected<std::string, Error> UpdateTeam(const domain::Team& team) = 0;
    virtual std::expected<void, Error> DeleteTeam(std::string_view id) = 0;
};

#endif /* ITEAM_DELEGATE_HPP */
