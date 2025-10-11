#ifndef DOMAIN_TOURNAMENT_HPP
#define DOMAIN_TOURNAMENT_HPP

#include <string>
#include <vector>

#include "domain/Group.hpp"
#include "domain/Match.hpp"

namespace domain {
    enum class TournamentType {
        ROUND_ROBIN, NFL
    };


    class TournamentFormat {
        int numberOfGroups;
        int maxTeamsPerGroup;
        TournamentType type;
    public:
        TournamentFormat(int numberOfGroups = 1, int maxTeamsPerGroup = 16, TournamentType tournamentType = TournamentType::ROUND_ROBIN) {
            this->numberOfGroups = numberOfGroups;
            this->maxTeamsPerGroup = maxTeamsPerGroup;
            this->type = tournamentType;
        }

        int NumberOfGroups() const {
            return this->numberOfGroups;
        }
        int & NumberOfGroups() {
            return this->numberOfGroups;
        }

        int MaxTeamsPerGroup() const {
            return this->maxTeamsPerGroup;
        }

        int & MaxTeamsPerGroup() {
            return this->maxTeamsPerGroup;
        }

        TournamentType Type() const {
            return this->type;
        }

        TournamentType & Type() {
            return this->type;
        }
    };

    class Tournament
    {
        std::string id;
        std::string name;
        TournamentFormat format;
        std::vector<Group> groups;
        std::vector<Match> matches;

    public:
        explicit Tournament(const std::string &name = "", const TournamentFormat& format = TournamentFormat()) {
            this->name = name;
            this->format = format;
        }

        [[nodiscard]] std::string Id() const {
            return this->id;
        }

        std::string& Id() {
            return this->id;
        }

        [[nodiscard]] std::string Name() const {
            return this->name;
        }

        std::string& Name() {
            return this->name;
        }

        [[nodiscard]] TournamentFormat Format() const {
            return this->format;
        }

        TournamentFormat & Format () {
            return this->format;
        }

        [[nodiscard]] std::vector<Group> & Groups() {
            return this->groups;
        }

        [[nodiscard]] std::vector<Match> Matches() const {
            return this->matches;
        }
    };
}
#endif