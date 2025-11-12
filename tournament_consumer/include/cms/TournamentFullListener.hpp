//
// Created by edgar on 11/12/25.
//

#ifndef TOURNAMENT_FULL_LISTENER_HPP
#define TOURNAMENT_FULL_LISTENER_HPP

#include "cms/QueueMessageListener.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include <memory>

// Listener que genera los 16 matches iniciales del Winner Bracket
// cuando un torneo alcanza 32 equipos

class TournamentFullListener : public QueueMessageListener {
    std::shared_ptr<IMatchRepository> matchRepo;
    std::shared_ptr<IGroupRepository> groupRepo;
public:
    TournamentFullListener(const std::shared_ptr<ConnectionManager>& cm,
                         const std::shared_ptr<IMatchRepository>& matchRepository,
                         const std::shared_ptr<IGroupRepository>& groupRepository);

protected:
    void processMessage(const std::string& message) override;
};

#endif // TOURNAMENT_FULL_LISTENER_HPP
