#ifndef SUPPORT_IGROUPDELEGATE_HPP
#define SUPPORT_IGROUPDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "model/Group.hpp"

// Stub de interfaz para tests
class IGroupDelegate {
public:
    virtual ~IGroupDelegate() = default;
    virtual std::expected<std::string, int> CreateGroup(const std::string& tid, const Group& group) = 0;
    virtual std::optional<Group> FindGroupById(const std::string& tid, const std::string& gid) = 0;
    virtual std::vector<Group> ListGroups(const std::string& tid) = 0;
    virtual std::expected<void, int> UpdateGroup(const std::string& tid, const std::string& gid, const Group& group) = 0;
    virtual std::expected<void, int> AddTeam(const std::string& tid, const std::string& gid, const std::string& teamId) = 0;
};

#endif // SUPPORT_IGROUPDELEGATE_HPP
