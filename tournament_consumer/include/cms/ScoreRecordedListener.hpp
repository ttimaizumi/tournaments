//
// Created by edgar on 11/11/25.
//
#ifndef LISTENER_SCORE_RECORDED_LISTENER_HPP
#define LISTENER_SCORE_RECORDED_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include <print>
#include <memory>

class ScoreRecordedListener : public QueueMessageListener {
    std::shared_ptr<IMatchRepository> matchRepo;

    void processMessage(const std::string& message) override {
        // message = matchId
        std::println("[consumer] score recorded for match: {}", message);
        // ejemplo: podrías leer el match si quieres
        // auto m = matchRepo->FindByTournamentAndId(tournamentId, message);
        // (no lo hacemos porque no tenemos tournamentId aqui; si lo necesitas, haz que el productor mande JSON)
    }

public:
    ScoreRecordedListener(const std::shared_ptr<ConnectionManager>& cm,
                          const std::shared_ptr<IMatchRepository>& repo)
        : QueueMessageListener(cm), matchRepo(repo) {}
    ~ScoreRecordedListener() override { Stop(); }
};

#endif // LISTENER_SCORE_RECORDED_LISTENER_HPP
