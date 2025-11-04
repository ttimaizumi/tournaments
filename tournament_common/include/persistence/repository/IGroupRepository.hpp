//
// Created by root on 9/27/25.
//

#ifndef COMMON_IGROUPREPOSITORY_HPP
#define COMMON_IGROUPREPOSITORY_HPP

#include "domain/Group.hpp"
#include "IRepository.hpp"


class IGroupRepository : public IRepository<domain::Group, std::string> {
public:
    virtual std::vector<std::shared_ptr<domain::Group>> FindByTournamentId(const std::string_view& tournamentId) = 0;
    virtual std::shared_ptr<domain::Group> FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) = 0;
    virtual std::shared_ptr<domain::Group> FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) = 0;
    virtual std::shared_ptr<domain::Group> FindByGroupIdAndTeamId(const std::string_view& groupId, const std::string_view& teamId) = 0;
    virtual void UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team> & team) = 0;
};
#endif //COMMON_IGROUPREPOSITORY_HPP