#ifndef SERVICE_IGROUP_DELEGATE_HPP
#define SERVICE_IGROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <expected>

#include "domain/Group.hpp"
#include "exception/Error.hpp"

class IGroupDelegate{
    public:
    virtual ~IGroupDelegate() = default;
    virtual std::expected<std::shared_ptr<domain::Group>, Error> GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) = 0;
    virtual std::expected<std::vector<std::shared_ptr<domain::Group>>, Error> GetGroups(const std::string_view& tournamentId) = 0;
    virtual std::expected<std::string, Error> CreateGroup(const std::string_view& tournamentId, const domain::Group& group) = 0;
    virtual std::expected<void, Error> UpdateGroup(const std::string_view& tournamentId, const domain::Group& group, const std::string_view& groupId) = 0;
    virtual std::expected<void, Error> UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams) = 0;
    virtual std::expected<void, Error> RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) = 0;
};

#endif /* SERVICE_IGROUP_DELEGATE_HPP */
