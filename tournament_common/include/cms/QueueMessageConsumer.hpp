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

class QueueMessageConsumer : public cms::MessageListener {
    std::shared_ptr<ConnectionManager> connectionManager;
    std::atomic<bool> running;
    std::thread worker;
    // std::shared_ptr<cms::Connection> connection;
    std::shared_ptr<cms::Session> session;
    std::shared_ptr<cms::MessageConsumer> messageConsumer;

    // void readMessage();
public:
    explicit QueueMessageConsumer(const std::shared_ptr<ConnectionManager>& connectionManager);
    ~QueueMessageConsumer();
    void Start(const std::string_view & queueName);
    void Stop();
    virtual void onMessage(const cms::Message* message);
};

inline QueueMessageConsumer::QueueMessageConsumer(const std::shared_ptr<ConnectionManager>& connectionManager) : connectionManager(connectionManager) {
    std::print("Created QueueMessageConsumer");
}

inline QueueMessageConsumer::~QueueMessageConsumer() {
    Stop();
}

inline void QueueMessageConsumer::Start(const std::string_view& queueName) {
    if (this->running)
        return;
    this->running = true;
    try {
        session = connectionManager->CreateSession();
        const auto destination = std::unique_ptr<cms::Queue>(session->createQueue(queueName.data()));
        auto consumer = std::unique_ptr<cms::MessageConsumer>(session->createConsumer(destination.get()));

        while (running) {
            std::unique_ptr<cms::Message> message(consumer->receive(1500));
            if (message) {
                if (auto text = dynamic_cast<cms::TextMessage*>(message.get())) {
                    std::print("message consumed: {}", text->getText());
                }
            }
        }
    } catch (const cms::CMSException& e) {

    }
}

inline void QueueMessageConsumer::Stop() {
    running = false;
    if (worker.joinable())
        worker.join();

    messageConsumer->close();
    session->close();
    // connection->close();
}

inline void QueueMessageConsumer::onMessage(const cms::Message* message) {

}

// inline void QueueMessageConsumer::readMessage() {
//     try {
//         if (this->running)
//             return;
//         session = connectionManager->CreateSession();
//         const auto destination = std::unique_ptr<cms::Destination>(session->createQueue(queueName));
//         auto consumer = std::unique_ptr<cms::MessageConsumer>(session->createConsumer(destination.get()));
//
//         while (running) {
//             std::unique_ptr<cms::Message> message(consumer->receive(1000));
//             if (message) {
//                 if (auto text = dynamic_cast<cms::TextMessage*>(message.get())) {
//                     std::print("message consumed: {}", text->getText());
//                 }
//             }
//         }
//     } catch (const cms::CMSException& e) {
//
//     }
// }

#endif //COMMON_QUEUE_MESSAGE_CONSUMER_HPP