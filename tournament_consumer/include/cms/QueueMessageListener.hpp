//
// Created by root on 9/24/25.
//
//
// QueueMessageListener.hpp - listener genérico para colas ActiveMQ
//

#ifndef COMMON_QUEUE_MESSAGE_CONSUMER_HPP
#define COMMON_QUEUE_MESSAGE_CONSUMER_HPP

#include <atomic>
#include <memory>
#include <thread>
#include <string>
#include <string_view>

#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/Destination.h>
#include <cms/MessageConsumer.h>
#include <cms/Message.h>
#include <cms/TextMessage.h>
#include <cms/CMSException.h>

#include <print>

#include "cms/ConnectionManager.hpp"

class QueueMessageListener {
protected:
    std::shared_ptr<ConnectionManager> connectionManager;
    std::shared_ptr<cms::Connection>   connection;
    std::shared_ptr<cms::Session>      session;
    std::unique_ptr<cms::MessageConsumer> messageConsumer;

    std::atomic<bool> running{false};
    std::thread       worker;

    // Cada listener hijo implementa esto
    virtual void processMessage(const std::string& message) = 0;

public:
    explicit QueueMessageListener(const std::shared_ptr<ConnectionManager>& cm)
        : connectionManager(cm) {}

    virtual ~QueueMessageListener() {
        Stop();
    }

    // Empieza a escuchar una cola
    void Start(const std::string_view& queueName) {
        if (running) {
            std::println("[QueueMessageListener] Already running");
            return;
        }

        // Obtenemos la conexión creada por ConnectionManager
        connection = connectionManager->Connection();
        if (!connection) {
            std::println("[QueueMessageListener] No connection available");
            return;
        }

        // Creamos sesión
        session = connectionManager->CreateSession();
        if (!session) {
            std::println("[QueueMessageListener] Unable to create session");
            return;
        }

        // Creamos destino y consumer
        std::unique_ptr<cms::Destination> dest(
            session->createQueue(std::string(queueName).c_str())
        );
        messageConsumer.reset(session->createConsumer(dest.get()));

        running = true;

        // Hilo worker que recibe mensajes en loop
        worker = std::thread([this] {
            std::println("[QueueMessageListener] started worker thread");
            while (running) {
                try {
                    // espera hasta 1s por mensaje
                    std::unique_ptr<cms::Message> msg(messageConsumer->receive(1000));
                    if (!msg) continue;

                    auto* text = dynamic_cast<cms::TextMessage*>(msg.get());
                    if (!text) continue;

                    const std::string payload = text->getText();
                    processMessage(payload);
                } catch (const cms::CMSException& e) {
                    std::println("[QueueMessageListener] CMSException: {}", e.getMessage());
                } catch (const std::exception& e) {
                    std::println("[QueueMessageListener] exception: {}", e.what());
                }
            }
            std::println("[QueueMessageListener] worker thread stopped");
        });
    }

    // Detener el listener y limpiar recursos
    void Stop() {
        // Si nunca se arrancó, no hacemos nada peligroso
        if (!running && !worker.joinable()) {
            return;
        }

        running = false;

        if (worker.joinable()) {
            try {
                worker.join();
            } catch (...) {
                // ignoramos errores al hacer join
            }
        }

        if (messageConsumer) {
            try {
                messageConsumer->close();
            } catch (...) {
            }
            messageConsumer.reset();
        }

        if (session) {
            try {
                session->close();
            } catch (...) {
            }
            session.reset();
        }

        // La conexión global la maneja ConnectionManager; no la cerramos aquí
    }
};

#endif // COMMON_QUEUE_MESSAGE_CONSUMER_HPP
