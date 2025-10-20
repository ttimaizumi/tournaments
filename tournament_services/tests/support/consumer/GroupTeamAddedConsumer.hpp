#ifndef SUPPORT_GROUPTEAMADDEDCONSUMER_HPP
#define SUPPORT_GROUPTEAMADDEDCONSUMER_HPP

#include <string>
#include <vector>
#include <expected>
#include "domain/Match.hpp"

// Stub de GroupTeamAddedConsumer para tests
// Escucha evento "team-added" y crea partidos round-robin cuando el grupo tiene 4 equipos
template<typename GroupRepo, typename MatchRepo>
class GroupTeamAddedConsumerImpl {
private:
    GroupRepo* groupRepo_;
    MatchRepo* matchRepo_;

public:
    GroupTeamAddedConsumerImpl(GroupRepo& groupRepo, MatchRepo& matchRepo)
        : groupRepo_(&groupRepo), matchRepo_(&matchRepo) {}

    // Procesa el evento de equipo agregado
    // Si el grupo ahora tiene 4 equipos, crea 6 partidos round-robin
    std::expected<int, int> OnTeamAdded(const std::string& tournamentId,
                                       const std::string& groupId,
                                       const std::string& teamId) {
        // Obtener tamaño del grupo
        int groupSize = groupRepo_->GroupSize(tournamentId, groupId);

        // Si no tiene exactamente 4 equipos, no hacer nada
        if (groupSize != 4) {
            return 0;  // 0 partidos creados
        }

        // Obtener la lista de equipos del grupo
        auto teams = groupRepo_->GetTeamsInGroup(tournamentId, groupId);
        if (teams.size() != 4) {
            return std::unexpected{500};
        }

        // Crear partidos round-robin (cada equipo juega contra cada uno)
        // Con 4 equipos: C(4,2) = 6 partidos
        int matchesCreated = 0;
        for (size_t i = 0; i < teams.size(); i++) {
            for (size_t j = i + 1; j < teams.size(); j++) {
                Match m;
                m.tournamentId = tournamentId;
                m.groupId = groupId;
                m.homeTeamId = teams[i];
                m.awayTeamId = teams[j];
                m.phase = "GROUP";
                m.homeScore = 0;
                m.awayScore = 0;
                m.isFinished = false;

                auto result = matchRepo_->Insert(tournamentId, m);
                if (result) {
                    matchesCreated++;
                }
            }
        }

        // Debe haber creado exactamente 6 partidos
        if (matchesCreated != 6) {
            return std::unexpected{500};
        }

        return matchesCreated;
    }
};

// Guía de deducción para CTAD
template<typename GR, typename MR>
GroupTeamAddedConsumerImpl(GR&, MR&) -> GroupTeamAddedConsumerImpl<GR, MR>;

#define GroupTeamAddedConsumer GroupTeamAddedConsumerImpl

#endif
