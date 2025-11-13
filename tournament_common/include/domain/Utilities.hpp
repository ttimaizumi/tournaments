#ifndef DOMAIN_UTILITIES_HPP
#define DOMAIN_UTILITIES_HPP

#include <nlohmann/json.hpp>
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"

namespace domain {

    inline void to_json(nlohmann::json& json, const Team& team) {
        json = {{"id", team.Id}, {"name", team.Name}};
    }

    inline void from_json(const nlohmann::json& json, Team& team) {
        if(json.contains("id")) {
            json.at("id").get_to(team.Id);
        }
        json.at("name").get_to(team.Name);
    }

    inline void from_json(const nlohmann::json& json, std::vector<Team>& teams) {
        for (auto j = json.begin(); j != json.end(); ++j) {
            Team team;
            if(j.value().contains("id")) {
                j.value().at("id").get_to(team.Id);
            }
            if(j.value().contains("name")) {
                j.value().at("name").get_to(team.Name);
            }
            teams.push_back(team);
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Team>& team) {
        json = nlohmann::basic_json();
        json["name"] = team->Name;

        if (!team->Id.empty()) {
            json["id"] = team->Id;
        }
    }

    inline TournamentType fromString(std::string_view type) {
        if (type == "ROUND_ROBIN")
            return TournamentType::ROUND_ROBIN;
        if (type == "NFL")
            return TournamentType::NFL;
        if (type == "DOUBLE_ELIMINATION")
            return TournamentType::DOUBLE_ELIMINATION;

        return TournamentType::ROUND_ROBIN;
    }

    inline void from_json(const nlohmann::json& json, TournamentFormat& format) {
        if(json.contains("maxTeamsPerGroup"))
            json.at("maxTeamsPerGroup").get_to(format.MaxTeamsPerGroup());
        if(json.contains("numberOfGroups"))
            json.at("numberOfGroups").get_to(format.NumberOfGroups());
        if(json.contains("type"))
            format.Type() = fromString(json["type"].get<std::string>());
    }

    inline void to_json(nlohmann::json& json, const TournamentFormat& format) {
        json = {{"maxTeamsPerGroup", format.MaxTeamsPerGroup()}, {"numberOfGroups", format.NumberOfGroups()}};
        switch (format.Type()) {
            case TournamentType::ROUND_ROBIN:
                json["type"] = "ROUND_ROBIN";
                break;
            case TournamentType::NFL:
                json["type"] = "NFL";
                break;
            case TournamentType::DOUBLE_ELIMINATION:
                json["type"] = "DOUBLE_ELIMINATION";
                break;
            default:
                json["type"] = "ROUND_ROBIN";
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Tournament>& tournament) {
        json = {{"name", tournament->Name()}};
        if (!tournament->Id().empty()) {
            json["id"] = tournament->Id();
        }
        json["format"] = tournament->Format();
    }

    inline void from_json(const nlohmann::json& json, std::shared_ptr<Tournament>& tournament) {
        if(json.contains("id")) {
            tournament->Id() = json["id"].get<std::string>();
        }
        json["name"].get_to(tournament->Name());
        if (json.contains("format"))
            json.at("format").get_to(tournament->Format());
    }

    inline void to_json(nlohmann::json& json, const Tournament& tournament) {
        json = {{"name", tournament.Name()}};
        if (!tournament.Id().empty()) {
            json["id"] = tournament.Id();
        }
        json["format"] = tournament.Format();
    }

    inline void from_json(const nlohmann::json& json, Tournament& tournament) {
        if(json.contains("id")) {
            tournament.Id() = json["id"].get<std::string>();
        }
        json["name"].get_to(tournament.Name());
        if (json.contains("format"))
            json.at("format").get_to(tournament.Format());
    }

    inline void from_json(const nlohmann::json& json, Group& group) {
        if(json.contains("id")) {
            group.Id() = json["id"].get<std::string>();
        }
        if(json.contains("tournamentId")) {
            group.TournamentId() = json["tournamentId"].get<std::string>();
        }
        json["name"].get_to(group.Name());
        json["teams"].get_to(group.Teams());
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Group>& group) {
        json["name"] = group->Name();
        json["tournamentId"] = group->TournamentId();
        if (!group->Id().empty()) {
            json["id"] = group->Id();
        }
        json["teams"] = group->Teams();
    }

    inline void to_json(nlohmann::json& json, const std::vector<std::shared_ptr<Group>>& groups) {
        json = nlohmann::json::array();
        for (const auto& group : groups) {
            auto jsonGroup = nlohmann::json();
            jsonGroup["name"] = group->Name();
            jsonGroup["tournamentId"] = group->TournamentId();
            if (!group->Id().empty()) {
                jsonGroup["id"] = group->Id();
            }
            jsonGroup["teams"] = group->Teams();
            json.push_back(jsonGroup);
        }
    }

    inline void to_json(nlohmann::json& json, const Group& group) {
        json["name"] = group.Name();
        json["tournamentId"] = group.TournamentId();
        if (!group.Id().empty()) {
            json["id"] = group.Id();
        }
        json["teams"] = group.Teams();
    }
    // Score -> json
    inline void to_json(nlohmann::json& j, const Score& s)
    {
        j = {
            {"home",    s.homeTeamScore},
            {"visitor", s.visitorTeamScore}
        };
    }

    // json -> Score
    inline void from_json(const nlohmann::json& j, Score& s)
    {
        j.at("home").get_to(s.homeTeamScore);
        j.at("visitor").get_to(s.visitorTeamScore);
    }

    // Match -> json (solo ids y score; los nombres los armara el servicio)
    inline void to_json(nlohmann::json& j, const Match& m)
    {
        j["homeTeamId"]    = m.HomeTeamId();
        j["visitorTeamId"] = m.VisitorTeamId();
        j["score"]         = m.MatchScore();
    }

    // json -> Match
    inline void from_json(const nlohmann::json& j, Match& m)
    {
        if (j.contains("homeTeamId"))
            j.at("homeTeamId").get_to(m.HomeTeamId());

        if (j.contains("visitorTeamId"))
            j.at("visitorTeamId").get_to(m.VisitorTeamId());

        if (j.contains("score"))
            j.at("score").get_to(m.MatchScore());
    }

}



#endif /* FC7CD637_41CC_48DE_8D8A_BC2CFC528D72 */
