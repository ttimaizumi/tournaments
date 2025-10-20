#ifndef SUPPORT_MATCHFINISHEDCONSUMER_HPP
#define SUPPORT_MATCHFINISHEDCONSUMER_HPP

#include <string>
#include <expected>
#include "domain/Match.hpp"
#include "domain/Standing.hpp"

// Stub de MatchFinishedConsumer para tests
// Escucha evento "match-finished" y actualiza tabla de posiciones
template<typename MatchRepo, typename StandingRepo>
class MatchFinishedConsumerImpl {
private:
    MatchRepo* matchRepo_;
    StandingRepo* standingRepo_;

public:
    MatchFinishedConsumerImpl(MatchRepo& matchRepo, StandingRepo& standingRepo)
        : matchRepo_(&matchRepo), standingRepo_(&standingRepo) {}

    // Procesa el evento de partido finalizado
    // Actualiza la tabla de posiciones con el resultado
    std::expected<void, int> OnMatchFinished(const std::string& matchId) {
        // Obtener partido
        auto match = matchRepo_->FindById(matchId);
        if (!match) {
            return std::unexpected{404};
        }

        // Verificar que el partido esté finalizado
        if (!match->isFinished) {
            return std::unexpected{400};
        }

        // Solo actualizar standings para fase de grupos
        if (match->phase != "GROUP") {
            return {};  // En eliminación no hay tabla de posiciones
        }

        // Obtener standings de ambos equipos
        auto homeStanding = standingRepo_->FindByTeam(match->tournamentId,
                                                      match->groupId,
                                                      match->homeTeamId);
        auto awayStanding = standingRepo_->FindByTeam(match->tournamentId,
                                                      match->groupId,
                                                      match->awayTeamId);

        if (!homeStanding || !awayStanding) {
            return std::unexpected{404};
        }

        // Actualizar estadísticas del equipo local
        homeStanding->matchesPlayed++;
        homeStanding->goalsFor += match->homeScore;
        homeStanding->goalsAgainst += match->awayScore;
        homeStanding->goalDifference = homeStanding->goalsFor - homeStanding->goalsAgainst;

        // Actualizar estadísticas del equipo visitante
        awayStanding->matchesPlayed++;
        awayStanding->goalsFor += match->awayScore;
        awayStanding->goalsAgainst += match->homeScore;
        awayStanding->goalDifference = awayStanding->goalsFor - awayStanding->goalsAgainst;

        // Determinar resultado y asignar puntos
        if (match->homeScore > match->awayScore) {
            // Victoria local
            homeStanding->wins++;
            homeStanding->points += 3;
            awayStanding->losses++;
        } else if (match->awayScore > match->homeScore) {
            // Victoria visitante
            awayStanding->wins++;
            awayStanding->points += 3;
            homeStanding->losses++;
        } else {
            // Empate
            homeStanding->draws++;
            homeStanding->points += 1;
            awayStanding->draws++;
            awayStanding->points += 1;
        }

        // Guardar cambios
        if (!standingRepo_->Update(*homeStanding) || !standingRepo_->Update(*awayStanding)) {
            return std::unexpected{500};
        }

        return {};
    }
};

// Guía de deducción para CTAD
template<typename MR, typename SR>
MatchFinishedConsumerImpl(MR&, SR&) -> MatchFinishedConsumerImpl<MR, SR>;

#define MatchFinishedConsumer MatchFinishedConsumerImpl

#endif
