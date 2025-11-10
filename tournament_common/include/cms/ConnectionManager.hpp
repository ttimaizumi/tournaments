//
// Created by tomas on 9/14/25.
//

#ifndef SERVICES_CONNECTION_MANAGER_HPP
#define SERVICES_CONNECTION_MANAGER_HPP

#include <cms/Connection.h>
#include <cms/Session.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <memory>
#include <print>

class ConnectionManager {
public:
    void initialize(const std::string_view& brokerURI) {
        std::println("Initializing ConnectionManager with broker: {}", brokerURI);
        factory = std::make_unique<activemq::core::ActiveMQConnectionFactory>(brokerURI.data());
        std::println("Creating connection...");
        connection = std::shared_ptr<cms::Connection>(factory->createConnection());
        std::println("Starting connection...");
        connection->start();
        std::println("Connection started successfully");
    }

    [[nodiscard]] std::shared_ptr<cms::Connection> Connection() const { return connection; }

    [[nodiscard]] std::shared_ptr<cms::Session> CreateSession() const {
        return std::shared_ptr<cms::Session>(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
    }

private:
    std::unique_ptr<activemq::core::ActiveMQConnectionFactory> factory;
    std::shared_ptr<cms::Connection> connection;
};

#endif //SERVICES_CONNECTION_MANAGER_HPP