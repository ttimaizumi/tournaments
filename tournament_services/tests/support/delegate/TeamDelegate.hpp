#ifndef SUPPORT_TEAMDELEGATE_HPP
#define SUPPORT_TEAMDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "model/Team.hpp"

// Stub de TeamDelegate para tests - envuelve el repositorio mockeado
template<typename TeamRepositoryMock>
class TeamDelegate {
public:
    explicit TeamDelegate(TeamRepositoryMock& repository) : repository_(repository) {}

    std::expected<std::string, int> Create(const Team& team) {
        auto result = repository_.Insert(team);
        if (result) {
            return *result;
        }
        return std::unexpected{409};
    }

    std::optional<Team> FindById(const std::string& id) {
        return repository_.FindById(id);
    }

    std::vector<Team> List() {
        return repository_.List();
    }

    std::expected<void, int> Update(const std::string& id, const Team& team) {
        if (repository_.Update(id, team)) {
            return {};
        }
        return std::unexpected{404};
    }

private:
    TeamRepositoryMock& repository_;
};

#endif // SUPPORT_TEAMDELEGATE_HPP
