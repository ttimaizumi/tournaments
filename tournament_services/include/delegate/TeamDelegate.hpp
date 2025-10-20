//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_TESTDELEGATE_HPP
#define RESTAPI_TESTDELEGATE_HPP
#include <memory>
#include <expected>

#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"
#include "ITeamDelegate.hpp"

class TeamDelegate : public ITeamDelegate {
    std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepository;
    public:
    explicit TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository);
    std::shared_ptr<domain::Team> GetTeam(std::string_view id) override;
    std::vector<std::shared_ptr<domain::Team>> GetAllTeams() override;
    std::expected<std::string, std::string> SaveTeam( const domain::Team& team) override;
    std::expected<std::string, std::string> UpdateTeam(const domain::Team& team) override;
};


#endif //RESTAPI_TESTDELEGATE_HPP