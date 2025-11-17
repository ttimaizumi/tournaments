//
// Created by edgar on 11/11/25.
//
#ifndef LISTENER_SCORE_RECORDED_LISTENER_HPP
#define LISTENER_SCORE_RECORDED_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include "persistence/repository/IMatchRepository.hpp"

#include <nlohmann/json.hpp>
#include <print>
#include <memory>
#include <string>
#include <exception>

// Consumidor para el evento "match.score-recorded".


class ScoreRecordedListener : public QueueMessageListener {
    std::shared_ptr<IMatchRepository> matchRepo;

public:
    ScoreRecordedListener(const std::shared_ptr<ConnectionManager>& cm,
                          const std::shared_ptr<IMatchRepository>& repo);
    ~ScoreRecordedListener() override;

protected:
    void processMessage(const std::string& message) override;
};

inline ScoreRecordedListener::ScoreRecordedListener(
        const std::shared_ptr<ConnectionManager>& cm,
        const std::shared_ptr<IMatchRepository>& repo)
    : QueueMessageListener(cm)
    , matchRepo(repo) {
}

inline ScoreRecordedListener::~ScoreRecordedListener() {
    Stop();
}

inline void ScoreRecordedListener::processMessage(const std::string& message) {
    using nlohmann::json;

    std::println("[consumer] match.score-recorded recibido: {}", message);


    if (message.empty() || message.front() != '{') {
        std::println("[consumer] mensaje sin JSON (solo matchId). "
                     "Se registra y no se ejecuta la logica avanzada.");
        if (!matchRepo) {
            std::println("[consumer] matchRepo no configurado, solo logging.");
        }
        return;
    }


    try {
        auto payload = json::parse(message);

        std::string tournamentId = payload.value("tournamentId", "");
        std::string matchId      = payload.value("matchId", "");
        std::string bracket      = payload.value("bracket", "winner");
        int         round        = payload.value("round", 1);

        int homeScore    = 0;
        int visitorScore = 0;
        if (payload.contains("score") && payload["score"].is_object()) {
            homeScore    = payload["score"].value("home", 0);
            visitorScore = payload["score"].value("visitor", 0);
        }

        if (tournamentId.empty() || matchId.empty()) {
            std::println("[consumer] JSON de score-recorded sin ids suficientes, se ignora.");
            return;
        }

        // Determinar ganador / perdedor solo a nivel de slots
        std::string winnerSlot = (homeScore > visitorScore) ? "home"    : "visitor";
        std::string loserSlot  = (homeScore > visitorScore) ? "visitor" : "home";

        std::println("[consumer] (demo) torneo={} match={} bracket={} round={} score {}-{} winner slot={}",
                     tournamentId, matchId, bracket, round, homeScore, visitorScore, winnerSlot);

        bool isLoserBracket = (bracket == "loser");
        int  nextRound      = round + 1;
        std::string nextBracket = bracket;

        // Ejemplo simple de logica repetida:
        // - Si estamos en winner bracket y el JSON dice que es final,
        //   marcariamos el torneo como concluido.
        // - Si no, "buscariamos" el siguiente partido y asignariamos al ganador.
        bool isFinalFlag = payload.value("isFinal", false);

        if (!isLoserBracket && isFinalFlag) {
            nextBracket = "completed";
        }

        if (nextBracket == "completed") {
            std::println("[consumer] (demo) torneo concluido. "
                         "Aqui se podria usar matchRepo para marcar el torneo como finalizado.");
            // Ejemplo de uso minimo de matchRepo para que no quede totalmente sin usar:
            if (!matchRepo) {
                std::println("[consumer] matchRepo es null, no se actualiza nada en BD (solo demo).");
            }
        } else {
            std::println("[consumer] (demo) aqui se buscaria el siguiente partido de {} round {} "
                         "y se asignaria al ganador en BD.",
                         nextBracket, nextRound);
            std::println("[consumer] (demo) tambien se podria mover al perdedor al bracket de perdedores si aplica.");
        }

    } catch (const std::exception& e) {
        std::println("[consumer] error al procesar mensaje score-recorded como JSON: {}", e.what());
    }
}

#endif //LISTENER_SCORE_RECORDED_LISTENER_HPP
