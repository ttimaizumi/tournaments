#ifndef SUPPORT_ELIMINATIONDELEGATE_HPP
#define SUPPORT_ELIMINATIONDELEGATE_HPP

#include <string>
#include <vector>
#include <map>
#include <expected>
#include "domain/Match.hpp"
#include "domain/Standing.hpp"

// Stub de EliminationDelegate para tests - Lógica de cruces del Mundial
template<typename StandingRepo, typename MatchRepo>
class EliminationDelegateImpl {
private:
    StandingRepo* standingRepo_;
    MatchRepo* matchRepo_;

    // Cruces de octavos según reglas del Mundial
    // Formato: {posición_local, posición_visitante}
    const std::vector<std::pair<std::string, std::string>> MUNDIAL_BRACKETS = {
        {"A1", "H2"}, // Match 1
        {"B2", "G1"}, // Match 2
        {"C1", "F2"}, // Match 3
        {"D2", "E1"}, // Match 4
        {"D1", "E2"}, // Match 5
        {"C2", "F1"}, // Match 6
        {"B1", "G2"}, // Match 7
        {"A2", "H1"}  // Match 8
    };

public:
    EliminationDelegateImpl(StandingRepo& standingRepo, MatchRepo& matchRepo)
        : standingRepo_(&standingRepo), matchRepo_(&matchRepo) {}

    // Crea los partidos de octavos de final con cruces correctos del Mundial
    std::expected<int, int> CreateRoundOf16(const std::string& tournamentId) {
        // Obtener top 2 de cada grupo (A-H)
        std::map<std::string, std::vector<std::string>> qualifiedTeams;

        for (char groupLetter = 'A'; groupLetter <= 'H'; groupLetter++) {
            std::string groupId = std::string(1, groupLetter);
            auto standings = standingRepo_->GetTopTeams(tournamentId, groupId, 2);

            if (standings.size() != 2) {
                return std::unexpected{400};  // Grupo no tiene 2 equipos clasificados
            }

            qualifiedTeams[groupId] = {standings[0].teamId, standings[1].teamId};
        }

        // Crear los 8 partidos de octavos según cruces del Mundial
        int matchesCreated = 0;

        for (const auto& bracket : MUNDIAL_BRACKETS) {
            // Parsear posición (ej: "A1" -> grupo A, posición 1)
            char homeGroup = bracket.first[0];
            int homePos = bracket.first[1] - '0';
            char awayGroup = bracket.second[0];
            int awayPos = bracket.second[1] - '0';

            std::string homeGroupId(1, homeGroup);
            std::string awayGroupId(1, awayGroup);

            // Obtener equipos (posición 1 = índice 0, posición 2 = índice 1)
            std::string homeTeamId = qualifiedTeams[homeGroupId][homePos - 1];
            std::string awayTeamId = qualifiedTeams[awayGroupId][awayPos - 1];

            // Crear partido de eliminación
            Match m;
            m.tournamentId = tournamentId;
            m.groupId = "";  // Sin grupo en eliminación
            m.homeTeamId = homeTeamId;
            m.awayTeamId = awayTeamId;
            m.phase = "ELIMINATION";
            m.round = "ROUND_OF_16";
            m.homeScore = 0;
            m.awayScore = 0;
            m.isFinished = false;

            auto result = matchRepo_->Insert(tournamentId, m);
            if (result) {
                matchesCreated++;
            }
        }

        // Debe haber creado exactamente 8 partidos
        if (matchesCreated != 8) {
            return std::unexpected{500};
        }

        return matchesCreated;
    }

    // Verifica que un cruce sea válido según reglas del Mundial
    bool IsValidBracket(const std::string& homePosition, const std::string& awayPosition) {
        for (const auto& bracket : MUNDIAL_BRACKETS) {
            if (bracket.first == homePosition && bracket.second == awayPosition) {
                return true;
            }
        }
        return false;
    }

    // Obtiene todos los cruces válidos del Mundial
    std::vector<std::pair<std::string, std::string>> GetMundialBrackets() const {
        return MUNDIAL_BRACKETS;
    }
};

// Guía de deducción para CTAD
template<typename SR, typename MR>
EliminationDelegateImpl(SR&, MR&) -> EliminationDelegateImpl<SR, MR>;

// Alias para uso en tests
#define EliminationDelegate EliminationDelegateImpl

#endif // SUPPORT_ELIMINATIONDELEGATE_HPP
