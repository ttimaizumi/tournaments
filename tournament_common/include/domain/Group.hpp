#ifndef DOMAIN_GROUP_HPP
#define DOMAIN_GROUP_HPP

#include <string>
#include <vector>
#include "domain/Team.hpp"

namespace domain {

    class Group {
        std::string id;
        std::string name;
        std::string tournamentId;
        std::vector<Team> teams;

    public:
        explicit Group(const std::string_view& name = "", const std::string_view& id = "")
            : id(id), name(name) {}

        [[nodiscard]] const std::string& Id() const { return id; }
        [[nodiscard]] const std::string& Name() const { return name; }
        [[nodiscard]] const std::string& TournamentId() const { return tournamentId; }
        [[nodiscard]] const std::vector<Team>& Teams() const { return teams; }

        std::string& MutableId() { return id; }
        std::string& MutableName() { return name; }
        std::string& MutableTournamentId() { return tournamentId; }
        std::vector<Team>& MutableTeams() { return teams; }

        void SetId(const std::string& newId) { id = newId; }
        void SetName(const std::string& newName) { name = newName; }
        void SetTournamentId(const std::string& tid) { tournamentId = tid; }
        void SetTeams(const std::vector<Team>& newTeams) { teams = newTeams; }
    };

}  // namespace domain

#endif
