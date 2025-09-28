#ifndef SERVICE_IGROUP_DELEGATE_HPP
#define SERVICE_IGROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <expected>

#include "domain/Group.hpp"

class IGroupDelegate{
public:
    virtual ~IGroupDelegate() = default;
    virtual std::expected<std::string, std::string> CreateGroup(const std::string_view& tournamentId, const domain::Group& group) = 0;
    virtual std::expected<std::vector<domain::Group>, std::string> GetGroups(const std::string_view& tournamentId) = 0;
    virtual std::expected<domain::Group, std::string> GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) = 0;
    virtual std::expected<void, std::string> UpdateGroup(const std::string_view& tournamentId, const domain::Group& group) = 0;
    virtual std::expected<void, std::string> RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) = 0;
};

#endif /* SERVICE_IGROUP_DELEGATE_HPP */
