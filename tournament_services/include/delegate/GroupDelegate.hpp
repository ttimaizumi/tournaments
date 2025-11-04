// include/delegate/GroupDelegate.hpp
#ifndef SERVICE_GROUP_DELEGATE_HPP
#define SERVICE_GROUP_DELEGATE_HPP

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "delegate/IGroupDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"

class GroupDelegate : public IGroupDelegate {
    std::shared_ptr<IRepository<domain::Tournament, std::string>>  tournamentRepository;
    std::shared_ptr<IGroupRepository>                               groupRepository;
    std::shared_ptr<IRepository<domain::Team, std::string_view>>         teamRepository;   // <- std::string para Id
    std::shared_ptr<IQueueMessageProducer>                          queueProducer;

public:
    GroupDelegate(const std::shared_ptr<IRepository<domain::Tournament, std::string>>& tRepo,
                  const std::shared_ptr<IGroupRepository>& gRepo,
                  const std::shared_ptr<IRepository<domain::Team, std::string_view>>& teamRepo, // <- std::string
                  const std::shared_ptr<IQueueMessageProducer>& producer);

    std::expected<std::string, std::string>
    CreateGroup(const std::string_view& tournamentId, const domain::Group& group) override;

    std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
    GetGroups(const std::string_view& tournamentId) override;

    std::expected<std::shared_ptr<domain::Group>, std::string>
    GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) override;

    std::expected<void, std::string>
    UpdateGroup(const std::string_view& tournamentId, const domain::Group& group) override;

    std::expected<void, std::string>
    RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) override;

    std::expected<void, std::string>
    UpdateTeams(const std::string_view& tournamentId,
                const std::string_view& groupId,
                const std::vector<domain::Team>& teams) override;
};

#endif // SERVICE_GROUP_DELEGATE_HPP
