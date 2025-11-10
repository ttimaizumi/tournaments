//
// Created for tournament match system
//

#ifndef LISTENER_SCOREUPDATELISTENER_HPP
#define LISTENER_SCOREUPDATELISTENER_HPP

#include <nlohmann/json.hpp>

#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/ScoreUpdateEvent.hpp"

class ScoreUpdateListener : public QueueMessageListener {
    void processMessage(const std::string& message) override;
    std::shared_ptr<MatchDelegate> matchDelegate;
public:
    ScoreUpdateListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                        const std::shared_ptr<MatchDelegate>& matchDelegate);
    ~ScoreUpdateListener() override;
};

inline ScoreUpdateListener::ScoreUpdateListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                                                 const std::shared_ptr<MatchDelegate>& matchDelegate)
    : QueueMessageListener(connectionManager), matchDelegate(matchDelegate) {
}

inline ScoreUpdateListener::~ScoreUpdateListener() {
    Stop();
}

inline void ScoreUpdateListener::processMessage(const std::string& message) {
    domain::ScoreUpdateEvent event = nlohmann::json::parse(message);
    matchDelegate->ProcessScoreUpdate(event);
}

#endif //LISTENER_SCOREUPDATELISTENER_HPP
