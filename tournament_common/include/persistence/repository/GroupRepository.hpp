//
// Created by root on 9/21/25.
//

#ifndef TOURNAMENTS_GROUPREPOSITORY_HPP
#define TOURNAMENTS_GROUPREPOSITORY_HPP

#include <string>
#include <memory>

#include "IGroupRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "domain/Group.hpp"
#include "domain/Utilities.hpp"

class GroupRepository : public IGroupRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider);
    std::shared_ptr<domain::Group> ReadById(std::string id) override;
    std::string Create (const domain::Group & entity) override;
    std::string Update (const domain::Group & entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Group>> ReadAll() override;
    std::vector<std::shared_ptr<domain::Group>> FindByTournamentId(const std::string_view& tournamentId) override;
    std::shared_ptr<domain::Group> FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) override;
    std::shared_ptr<domain::Group> FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) override;
    void UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team> & team) override;
};

#endif //TOURNAMENTS_GROUPREPOSITORY_HPP