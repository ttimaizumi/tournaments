#ifndef SUPPORT_ITEAMDELEGATE_HPP
#define SUPPORT_ITEAMDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "model/Team.hpp"

// Stub de interfaz para tests
class ITeamDelegate {
public:
    virtual ~ITeamDelegate() = default;
    virtual std::expected<std::string, int> Create(const Team& team) = 0;
    virtual std::optional<Team> FindById(const std::string& id) = 0;
    virtual std::vector<Team> List() = 0;
    virtual std::expected<void, int> Update(const std::string& id, const Team& team) = 0;
};

#endif // SUPPORT_ITEAMDELEGATE_HPP
