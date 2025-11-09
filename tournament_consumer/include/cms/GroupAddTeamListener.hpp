//
// Created by developer on 10/14/25.
//

#ifndef LISTENER_GROUPADDTEAM_LISTENER_HPP
#define LISTENER_GROUPADDTEAM_LISTENER_HPP

#include <nlohmann/json.hpp>

#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/TeamAddEvent.hpp"

class GroupAddTeamListener : public QueueMessageListener{
    void processMessage(const std::string& message) override;
    std::shared_ptr<MatchDelegate> matchDelegate;
public:
    GroupAddTeamListener(const std::shared_ptr<ConnectionManager> &connectionManager, const std::shared_ptr<MatchDelegate> &matchDelegate);
    ~GroupAddTeamListener() override;

};

inline GroupAddTeamListener::GroupAddTeamListener(const std::shared_ptr<ConnectionManager> &connectionManager, const std::shared_ptr<MatchDelegate> &matchDelegate)
    : QueueMessageListener(connectionManager), matchDelegate(matchDelegate) {
}

inline GroupAddTeamListener::~GroupAddTeamListener() {
    Stop();
}

inline void GroupAddTeamListener::processMessage(const std::string &message) {
    domain::TeamAddEvent event = nlohmann::json::parse(message);
    matchDelegate->ProcessTeamAddition(event);
}


#endif //LISTENER_GROUPADDTEAM_LISTENER_HPP