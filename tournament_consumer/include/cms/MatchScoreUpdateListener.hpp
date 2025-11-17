//
// Created by developer on 11/12/25.
//

#ifndef CONSUMER_MATCHSCOREUPDATELISTENER_HPP
#define CONSUMER_MATCHSCOREUPDATELISTENER_HPP

#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>

#include "cms/QueueMessageListener.hpp"
#include "event/ScoreUpdateEvent.hpp"
#include "delegate/MatchDelegate.hpp"

class MatchScoreUpdateListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate> matchDelegate;
    void processMessage(const std::string& message) override;

public:
    MatchScoreUpdateListener(const std::shared_ptr<ConnectionManager> &connectionManager, const std::shared_ptr<MatchDelegate> &matchDelegate);
    ~MatchScoreUpdateListener() override;
};

inline MatchScoreUpdateListener::MatchScoreUpdateListener(const std::shared_ptr<ConnectionManager> &connectionManager, const std::shared_ptr<MatchDelegate> &matchDelegate)
    : QueueMessageListener(connectionManager), matchDelegate(matchDelegate) {
}

inline MatchScoreUpdateListener::~MatchScoreUpdateListener() {
    Stop();
}

inline void MatchScoreUpdateListener::processMessage(const std::string& message) {
    std::cout << "[MatchScoreUpdateListener] Received message: " << message << std::endl;

    try {
        nlohmann::json json = nlohmann::json::parse(message);
        domain::ScoreUpdateEvent event = json.get<domain::ScoreUpdateEvent>();
        
        matchDelegate->ProcessScoreUpdate(event);
    } catch (const std::exception& e) {
        std::cout << "[MatchScoreUpdateListener] ERROR: " << e.what() << std::endl;
    }
}

#endif //CONSUMER_MATCHSCOREUPDATELISTENER_HPP
