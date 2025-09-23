//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_TESTDELEGATE_HPP
#define RESTAPI_TESTDELEGATE_HPP
#include <memory>

#include "ITeamDelegate.hpp"
#include "domain/Team.hpp"
#include "persistence/repository/IRepository.hpp"

class TeamDelegate : public ITeamDelegate {
  std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepository;

public:
  explicit TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository);
  std::shared_ptr<domain::Team> GetTeam(std::string_view id) override;
  std::vector<std::shared_ptr<domain::Team>> GetAllTeams() override;
  std::string_view CreateTeam(const domain::Team& team) override;
};

#endif  // RESTAPI_TESTDELEGATE_HPP