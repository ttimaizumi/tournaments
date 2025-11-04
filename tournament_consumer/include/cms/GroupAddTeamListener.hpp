//
// Created by developer on 10/14/25.
//

#ifndef LISTENER_GROUPADDTEAM_LISTENER_HPP
#define LISTENER_GROUPADDTEAM_LISTENER_HPP
#include "QueueMessageListener.hpp"

class GroupAddTeamListener : public QueueMessageListener{
    void processMessage(const std::string& message) override;
public:
    GroupAddTeamListener(const std::shared_ptr<ConnectionManager> &connectionManager);
    ~GroupAddTeamListener() override;

};

inline GroupAddTeamListener::GroupAddTeamListener(const std::shared_ptr<ConnectionManager> &connectionManager)
    : QueueMessageListener(connectionManager) {
}

inline GroupAddTeamListener::~GroupAddTeamListener() {
    Stop();
}

inline void GroupAddTeamListener::processMessage(const std::string &message) {

}


#endif //LISTENER_GROUPADDTEAM_LISTENER_HPP