#ifndef SUPPORT_STANDINGDELEGATE_HPP
#define SUPPORT_STANDINGDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include <algorithm>
#include "domain/Standing.hpp"

// Stub de StandingDelegate para tests - Mundial rules
template<typename StandingRepo>
class StandingDelegateImpl {
private:
    StandingRepo* repo_;

public:
    explicit StandingDelegateImpl(StandingRepo& repo) : repo_(&repo) {}

    std::optional<Standing> GetStanding(const std::string& tournamentId,
                                       const std::string& groupId,
                                       const std::string& teamId) {
        return repo_->FindByTeam(tournamentId, groupId, teamId);
    }

    std::vector<Standing> ListStandings(const std::string& tournamentId,
                                       const std::string& groupId) {
        return repo_->ListByGroup(tournamentId, groupId);
    }

    // Obtiene los top N equipos según reglas del Mundial:
    // 1. Puntos (victorias=3, empates=1, derrotas=0)
    // 2. Diferencia de goles
    // 3. Goles a favor
    std::vector<Standing> GetTopTeams(const std::string& tournamentId,
                                     const std::string& groupId,
                                     int limit) {
        auto standings = repo_->ListByGroup(tournamentId, groupId);

        // Ordenar según reglas del Mundial
        std::sort(standings.begin(), standings.end(),
            [](const Standing& a, const Standing& b) {
                // 1. Más puntos gana
                if (a.points != b.points) return a.points > b.points;
                // 2. Mayor diferencia de goles
                if (a.goalDifference != b.goalDifference)
                    return a.goalDifference > b.goalDifference;
                // 3. Más goles a favor
                return a.goalsFor > b.goalsFor;
            });

        // Retornar solo top N
        if (standings.size() > static_cast<size_t>(limit)) {
            standings.resize(limit);
        }

        return standings;
    }

    // Actualiza standings después de un partido finalizado
    std::expected<void, int> UpdateAfterMatch(const std::string& tournamentId,
                                             const std::string& groupId,
                                             const std::string& homeTeamId,
                                             const std::string& awayTeamId,
                                             int homeScore,
                                             int awayScore) {
        // Validar scores
        if (homeScore < 0 || awayScore < 0) {
            return std::unexpected{400};
        }

        // Obtener standings actuales
        auto homeStanding = repo_->FindByTeam(tournamentId, groupId, homeTeamId);
        auto awayStanding = repo_->FindByTeam(tournamentId, groupId, awayTeamId);

        if (!homeStanding || !awayStanding) {
            return std::unexpected{404};
        }

        // Actualizar estadísticas
        homeStanding->matchesPlayed++;
        homeStanding->goalsFor += homeScore;
        homeStanding->goalsAgainst += awayScore;
        homeStanding->goalDifference = homeStanding->goalsFor - homeStanding->goalsAgainst;

        awayStanding->matchesPlayed++;
        awayStanding->goalsFor += awayScore;
        awayStanding->goalsAgainst += homeScore;
        awayStanding->goalDifference = awayStanding->goalsFor - awayStanding->goalsAgainst;

        // Determinar resultado y actualizar puntos
        if (homeScore > awayScore) {
            // Victoria local
            homeStanding->wins++;
            homeStanding->points += 3;
            awayStanding->losses++;
        } else if (awayScore > homeScore) {
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
        if (!repo_->Update(*homeStanding) || !repo_->Update(*awayStanding)) {
            return std::unexpected{500};
        }

        return {};
    }
};

// Guía de deducción para CTAD
template<typename SR>
StandingDelegateImpl(SR&) -> StandingDelegateImpl<SR>;

// Alias para uso en tests
#define StandingDelegate StandingDelegateImpl

#endif // SUPPORT_STANDINGDELEGATE_HPP
