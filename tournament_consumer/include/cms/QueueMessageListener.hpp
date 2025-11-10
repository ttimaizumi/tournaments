//
// Created by root on 9/24/25.
//

#ifndef COMMON_QUEUE_MESSAGE_CONSUMER_HPP
#define COMMON_QUEUE_MESSAGE_CONSUMER_HPP


#include <atomic>
#include <memory>
#include <thread>
#include <cms/MessageConsumer.h>
#include <cms/Session.h>
#include <print>

#include "cms/ConnectionManager.hpp"

class QueueMessageListener {
    std::shared_ptr<ConnectionManager> connectionManager;
    std::atomic<bool> running;
    std::thread worker;
    std::shared_ptr<cms::Session> session;
    std::shared_ptr<cms::MessageConsumer> messageConsumer;

    virtual void processMessage(const std::string& message) = 0 ;
public:
    explicit QueueMessageListener(const std::shared_ptr<ConnectionManager>& connectionManager);
    virtual ~QueueMessageListener() = default;
    void Start(const std::string_view & queueName);
    void Stop();
};

inline QueueMessageListener::QueueMessageListener(const std::shared_ptr<ConnectionManager>& connectionManager) : connectionManager(connectionManager) {
    std::print("Created QueueMessageConsumer");
}

inline void QueueMessageListener::Start(const std::string_view& queueName) {
    if (this->running)
        return;
    this->running = true;
    try {
        std::println("Attempting to create session...");
        session = connectionManager->CreateSession();
        std::println("Session created, creating queue: {}", queueName);
        const auto destination = std::unique_ptr<cms::Queue>(session->createQueue(queueName.data()));
        auto consumer = std::unique_ptr<cms::MessageConsumer>(session->createConsumer(destination.get()));
        std::println("Consumer created successfully, starting message loop...");

        while (running) {
            std::unique_ptr<cms::Message> message(consumer->receive(1500));
            if (message) {
                if (auto text = dynamic_cast<cms::TextMessage*>(message.get())) {
                   processMessage(text->getText());
                }
            }
        }
    } catch (const cms::CMSException& e) {
        std::println("CMS Exception in Start: {}", e.getMessage());
    } catch (const std::exception& e) {
        std::println("Exception in Start: {}", e.what());
    }
}

inline void QueueMessageListener::Stop() {
    running = false;
    if (worker.joinable())
        worker.join();

    messageConsumer->close();
    session->close();
    // connection->close();
}

#endif //COMMON_QUEUE_MESSAGE_CONSUMER_HPP