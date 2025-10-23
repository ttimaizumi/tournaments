//
// Created by tomas on 9/9/25.
//

#ifndef SERVICE_MESSAGE_PRODUCER_HPP
#define SERVICE_MESSAGE_PRODUCER_HPP

#include <string_view>
#include <memory>

#include "IQueueMessageProducer.hpp"
#include "cms/ConnectionManager.hpp"

class QueueMessageProducer: public IQueueMessageProducer {
    std::shared_ptr<ConnectionManager> connectionManager;
public:
    explicit QueueMessageProducer(const std::shared_ptr<ConnectionManager>& connectionManager) : connectionManager(connectionManager){}

    void SendMessage(const std::string_view& message, const std::string_view& queue) override {
        auto session = connectionManager->CreateSession();
        const auto destination = std::unique_ptr<cms::Destination>(session->createQueue(queue.data()));
        auto producer = std::unique_ptr<cms::MessageProducer>(session->createProducer(destination.get()));
        producer->setDeliveryMode( cms::DeliveryMode::PERSISTENT );

        const auto brokerMessage = std::unique_ptr<cms::TextMessage>(session->createTextMessage(message.data()));
        producer->send(brokerMessage.get());
    }
};

#endif //SERVICE_MESSAGE_PRODUCER_HPP