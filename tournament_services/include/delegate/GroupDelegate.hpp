#ifndef SERVICE_GROUP_DELEGATE_HPP
#define SERVICE_GROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <expected>
#include <regex>

#include "IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "exception/Error.hpp"
#include "domain/Constants.hpp"
#include "cms/IQueueMessageProducer.hpp"

class GroupDelegate : public IGroupDelegate{
    std::shared_ptr<TournamentRepository> tournamentRepository;
    std::shared_ptr<IGroupRepository> groupRepository;
    std::shared_ptr<TeamRepository> teamRepository;
    std::shared_ptr<IQueueMessageProducer> messageProducer;

public:
    GroupDelegate(const std::shared_ptr<TournamentRepository>& tournamentRepository, const std::shared_ptr<IGroupRepository>& groupRepository, const std::shared_ptr<TeamRepository>& teamRepository, const std::shared_ptr<IQueueMessageProducer>& messageProducer);
    std::expected<std::shared_ptr<domain::Group>, Error> GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) override;
    std::expected<std::vector<std::shared_ptr<domain::Group>>, Error> GetGroups(const std::string_view& tournamentId) override;
    std::expected<std::string, Error> CreateGroup(const std::string_view& tournamentId, const domain::Group& group) override;
    std::expected<void, Error> UpdateGroup(const std::string_view& tournamentId, const domain::Group& group, const std::string_view& groupId) override;
    std::expected<void, Error> UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& team) override;
    std::expected<void, Error> RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) override;
};

#endif /* SERVICE_GROUP_DELEGATE_HPP */
