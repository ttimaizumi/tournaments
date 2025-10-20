#ifndef SUPPORT_MATCHDELEGATE_HPP
#define SUPPORT_MATCHDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "domain/Match.hpp"

// Stub de MatchDelegate para tests
template<typename MatchRepo, typename EventBus, typename TeamRepo = void*>
class MatchDelegateImpl {
private:
    MatchRepo* matchRepo_;
    EventBus* bus_;
    TeamRepo* teamRepo_;

public:
    // Constructor sin TeamRepo (retrocompatibilidad)
    MatchDelegateImpl(MatchRepo& repo, EventBus& bus)
        : matchRepo_(&repo), bus_(&bus), teamRepo_(nullptr) {}

    // Constructor con TeamRepo para validaciones extendidas
    MatchDelegateImpl(MatchRepo& repo, EventBus& bus, TeamRepo& teamRepo)
        : matchRepo_(&repo), bus_(&bus), teamRepo_(&teamRepo) {}

    std::expected<std::string, int> CreateMatch(const std::string& tournamentId, const Match& match) {
        // Validar que los equipos no estén vacíos en fase de grupos
        if (match.phase == "GROUP" && match.homeTeamId.empty()) {
            return std::unexpected{400};
        }

        // Si tenemos TeamRepo, validar que ambos equipos pertenezcan al mismo grupo
        if constexpr (!std::is_same_v<TeamRepo, void*>) {
            if (match.phase == "GROUP" && teamRepo_ != nullptr) {
                auto homeGroup = teamRepo_->GetTeamGroup(tournamentId, match.homeTeamId);
                auto awayGroup = teamRepo_->GetTeamGroup(tournamentId, match.awayTeamId);

                // Si algún equipo no está en un grupo, error 404
                if (!homeGroup || !awayGroup) {
                    return std::unexpected{404};
                }

                // Si están en grupos diferentes, error 422
                if (*homeGroup != *awayGroup) {
                    return std::unexpected{422};
                }

                // Si el grupo del match no coincide, error 422
                if (!match.groupId.empty() && *homeGroup != match.groupId) {
                    return std::unexpected{422};
                }
            }
        }

        auto result = matchRepo_->Insert(tournamentId, match);
        if (result) {
            bus_->Publish("match-created", *result);
            return *result;
        }
        return std::unexpected{409};
    }

    std::optional<Match> FindMatchById(const std::string& matchId) {
        return matchRepo_->FindById(matchId);
    }

    std::vector<Match> ListMatchesByGroup(const std::string& tournamentId, const std::string& groupId) {
        return matchRepo_->ListByGroup(tournamentId, groupId);
    }

    std::expected<void, int> UpdateScore(const std::string& matchId, int homeScore, int awayScore) {
        // Validar rango 0-10
        if (homeScore < 0 || homeScore > 10 || awayScore < 0 || awayScore > 10) {
            return std::unexpected{400};
        }

        auto match = matchRepo_->FindById(matchId);
        if (!match) {
            return std::unexpected{404};
        }

        // No permitir empates en eliminación
        if (match->phase == "ELIMINATION" && homeScore == awayScore) {
            return std::unexpected{422};
        }

        if (matchRepo_->UpdateScore(matchId, homeScore, awayScore)) {
            bus_->Publish("score-updated", matchId);
            return {};
        }
        return std::unexpected{500};
    }

    std::expected<void, int> FinishMatch(const std::string& matchId) {
        auto match = matchRepo_->FindById(matchId);
        if (!match) {
            return std::unexpected{404};
        }

        if (matchRepo_->FinishMatch(matchId)) {
            bus_->Publish("match-finished", matchId);
            return {};
        }
        return std::unexpected{500};
    }
};

// Guías de deducción para CTAD
template<typename MR, typename EB>
MatchDelegateImpl(MR&, EB&) -> MatchDelegateImpl<MR, EB, void*>;

template<typename MR, typename EB, typename TR>
MatchDelegateImpl(MR&, EB&, TR&) -> MatchDelegateImpl<MR, EB, TR>;

// Alias para uso en tests
#define MatchDelegate MatchDelegateImpl

#endif // SUPPORT_MATCHDELEGATE_HPP
