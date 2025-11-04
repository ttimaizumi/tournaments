#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <expected>

#include "domain/Team.hpp"
#include "persistence/repository/IRepository.hpp"
#include "exception/Error.hpp"
#include "delegate/ITeamDelegate.hpp"

class TeamDelegate : public ITeamDelegate { // changed: now implements ITeamDelegate
public:
    TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository);

    std::expected<std::vector<std::shared_ptr<domain::Team>>, Error> GetAllTeams() override;
    std::expected<std::shared_ptr<domain::Team>, Error> GetTeam(std::string_view id) override;
    std::expected<std::string, Error> CreateTeam(const domain::Team& team) override;
    std::expected<std::string, Error> UpdateTeam(const domain::Team& team) override;
    std::expected<void, Error> DeleteTeam(std::string_view id) override;

private:
    std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepository;
};