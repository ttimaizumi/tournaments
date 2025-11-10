#ifndef DOMAIN_MATCH_HPP
#define DOMAIN_MATCH_HPP

#include <string>
#include <optional>

namespace domain {
    enum class Winner { HOME, VISITOR };

    enum class Round {
        REGULAR,
        EIGHTHS,
        QUARTERS,
        SEMIS,
        FINAL
    };

    struct Score {
        int homeTeamScore;
        int visitorTeamScore;

        [[nodiscard]] Winner GetWinner() const {
            if (visitorTeamScore < homeTeamScore) {
                return Winner::HOME;
            }
            return Winner::VISITOR;
        }

        [[nodiscard]] bool IsTie() const {
            return homeTeamScore == visitorTeamScore;
        }

        [[nodiscard]] int GetGoalDifference(Winner team) const {
            if (team == Winner::HOME) {
                return homeTeamScore - visitorTeamScore;
            }
            return visitorTeamScore - homeTeamScore;
        }
    };

    class Match {
        std::string id;
        std::string tournamentId;
        std::string homeTeamId;
        std::string homeTeamName;
        std::string visitorTeamId;
        std::string visitorTeamName;
        Round round;
        std::optional<Score> score;

    public:
        Match() : round(Round::REGULAR) {}

        // ID getters/setters
        [[nodiscard]] const std::string& Id() const { return id; }
        std::string& Id() { return id; }
        void SetId(const std::string& newId) { id = newId; }

        // Tournament ID getters/setters
        [[nodiscard]] const std::string& TournamentId() const { return tournamentId; }
        std::string& TournamentId() { return tournamentId; }
        void SetTournamentId(const std::string& newTournamentId) { tournamentId = newTournamentId; }

        // Home team getters/setters
        [[nodiscard]] const std::string& HomeTeamId() const { return homeTeamId; }
        std::string& HomeTeamId() { return homeTeamId; }
        void SetHomeTeamId(const std::string& newHomeTeamId) { homeTeamId = newHomeTeamId; }

        [[nodiscard]] const std::string& HomeTeamName() const { return homeTeamName; }
        std::string& HomeTeamName() { return homeTeamName; }
        void SetHomeTeamName(const std::string& newHomeTeamName) { homeTeamName = newHomeTeamName; }

        // Visitor team getters/setters
        [[nodiscard]] const std::string& VisitorTeamId() const { return visitorTeamId; }
        std::string& VisitorTeamId() { return visitorTeamId; }
        void SetVisitorTeamId(const std::string& newVisitorTeamId) { visitorTeamId = newVisitorTeamId; }

        [[nodiscard]] const std::string& VisitorTeamName() const { return visitorTeamName; }
        std::string& VisitorTeamName() { return visitorTeamName; }
        void SetVisitorTeamName(const std::string& newVisitorTeamName) { visitorTeamName = newVisitorTeamName; }

        // Round getters/setters
        [[nodiscard]] Round GetRound() const { return round; }
        Round& GetRound() { return round; }
        void SetRound(Round newRound) { round = newRound; }

        // Score getters/setters
        [[nodiscard]] const std::optional<Score>& MatchScore() const { return score; }
        std::optional<Score>& MatchScore() { return score; }
        void SetScore(const Score& newScore) { score = newScore; }

        [[nodiscard]] bool HasScore() const { return score.has_value(); }
    };
}
#endif