#ifndef DOMAIN_MATCH_HPP
#define DOMAIN_MATCH_HPP

#include <string>
namespace domain {
    enum class Winner { HOME, VISITOR  };
    
    struct Score {
        int homeTeamScore = 0;
        int visitorTeamScore = 0;
        [[nodiscard]] Winner GetWinner() const {
            if (visitorTeamScore < homeTeamScore) {
                return Winner::HOME;
            }
            return Winner::VISITOR;
        }
    };
    
    class Match {
        /* data */
        std::string id;
        std::string name; // e.g., "W0", "W1", "L0", "L1", "F0", "F1"
        std::string tournamentId;
        std::string homeTeamId;
        std::string visitorTeamId;
        Score score;

    public:
        Match(/* args */){}

        [[nodiscard]] std::string Id() const {
            return  id;
        }

        std::string& Id() {
            return  id;
        }

        [[nodiscard]] std::string Name() const {
            return name;
        }

        std::string& Name() {
            return name;
        }

        [[nodiscard]] std::string TournamentId() const {
            return  tournamentId;
        }

        [[nodiscard]] std::string & TournamentId() {
            return  tournamentId;
        }

        [[nodiscard]] std::string HomeTeamId() const {
            return homeTeamId;
        }
        std::string & HomeTeamId() {
            return homeTeamId;
        }

        [[nodiscard]] std::string VisitorTeamId() const {
            return visitorTeamId;
        }

        std::string & VisitorTeamId() {
            return visitorTeamId;
        }

        Score & MatchScore() {
            return score;
        }

        [[nodiscard]] Score MatchScore() const {
            return score;
        }
    };
    
}
#endif