//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_TESTDELEGATE_HPP
#define RESTAPI_TESTDELEGATE_HPP

#include <memory>
#include <string_view>
#include <expected>

#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"
// Usa la ruta desde la raiz de include para consistencia
#include "delegate/ITeamDelegate.hpp"

class TeamDelegate : public ITeamDelegate {
    std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepository;

public:
    explicit TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository);

    std::shared_ptr<domain::Team> GetTeam(std::string_view id) override;
    std::vector<std::shared_ptr<domain::Team>> GetAllTeams() override;
    std::string_view SaveTeam(const domain::Team& team) override;

    //  implementacion requerida por ITeamDelegate
    bool UpdateTeam(const domain::Team& team) override;
    // variantes con std::expected para cumplir requisitos
    std::expected<std::string, std::string> SaveTeamEx(const domain::Team& team);
    std::expected<void, std::string> UpdateTeamEx(const domain::Team& team);
};

#endif // RESTAPI_TESTDELEGATE_HPP
