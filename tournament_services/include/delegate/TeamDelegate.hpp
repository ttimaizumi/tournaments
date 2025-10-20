#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <expected>

#include "domain/Team.hpp"
#include "persistence/repository/IRepository.hpp"
#include "exception/Error.hpp"

class TeamDelegate {
public:
    TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository);

    std::expected<std::vector<std::shared_ptr<domain::Team>>, Error> GetAllTeams();
    std::expected<std::shared_ptr<domain::Team>, Error> GetTeam(std::string_view id);
    std::expected<std::string, Error> CreateTeam(const domain::Team& team);
    std::expected<std::string, Error> UpdateTeam(const domain::Team& team);
    std::expected<void, Error> DeleteTeam(std::string_view id);

private:
    std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepository;
};