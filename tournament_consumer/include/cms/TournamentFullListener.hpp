//
// Created by edgar on 11/12/25.
//

#ifndef TOURNAMENT_FULL_LISTENER_HPP
#define TOURNAMENT_FULL_LISTENER_HPP

#include "cms/QueueMessageListener.hpp"
#include <memory>

// Por ahora este listener solo va a leer el evento tournament.full
// y escribir en consola. Después ya lo podemos hacer que cree grupos
// y matches si quieres, pero primero probamos el flujo de eventos.

class TournamentFullListener : public QueueMessageListener {
public:
    explicit TournamentFullListener(const std::shared_ptr<ConnectionManager>& cm);

protected:
    void processMessage(const std::string& message) override;
};

#endif // TOURNAMENT_FULL_LISTENER_HPP
