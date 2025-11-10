#ifndef DOMAIN_UTILITIES_HPP
#define DOMAIN_UTILITIES_HPP

#include <nlohmann/json.hpp>
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"
#include <regex>

static const std::regex ID_VALUE("[A-Za-z0-9\\-]+");

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
        if (type == "MUNDIAL")
            return TournamentType::MUNDIAL;

        return TournamentType::MUNDIAL;
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
            case TournamentType::MUNDIAL:
                json["type"] = "MUNDIAL";
                break;
            default:
                json["type"] = "MUNDIAL";
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

    inline void from_json(const nlohmann::json& json, domain::Group& group) {
        if (json.contains("id"))
            group.SetId(json["id"].get<std::string>());
        if (json.contains("tournamentId"))
            group.SetTournamentId(json["tournamentId"].get<std::string>());
        if (json.contains("name"))
            group.SetName(json["name"].get<std::string>());
        if (json.contains("teams"))
            group.SetTeams(json["teams"].get<std::vector<domain::Team>>());
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

    inline void to_json(nlohmann::json& json, const domain::Group& group) {
        json = nlohmann::json{
            {"id", group.Id()},
            {"tournamentId", group.TournamentId()},
            {"name", group.Name()},
            {"teams", group.Teams()}
        };
    }

    // Round serialization
    inline std::string roundToString(Round round) {
        switch (round) {
            case Round::REGULAR: return "regular";
            case Round::EIGHTHS: return "eighths";
            case Round::QUARTERS: return "quarters";
            case Round::SEMIS: return "semis";
            case Round::FINAL: return "final";
            default: return "regular";
        }
    }

    inline Round roundFromString(const std::string& str) {
        if (str == "eighths") return Round::EIGHTHS;
        if (str == "quarters") return Round::QUARTERS;
        if (str == "semis") return Round::SEMIS;
        if (str == "final") return Round::FINAL;
        return Round::REGULAR;
    }

    // Score serialization
    inline void to_json(nlohmann::json& json, const Score& score) {
        json = nlohmann::json{
            {"home", score.homeTeamScore},
            {"visitor", score.visitorTeamScore}
        };
    }

    inline void from_json(const nlohmann::json& json, Score& score) {
        json.at("home").get_to(score.homeTeamScore);
        json.at("visitor").get_to(score.visitorTeamScore);
    }

    // Match serialization
    inline void to_json(nlohmann::json& json, const Match& match) {
        json = nlohmann::json{
            {"home", {
                {"id", match.HomeTeamId()},
                {"name", match.HomeTeamName()}
            }},
            {"visitor", {
                {"id", match.VisitorTeamId()},
                {"name", match.VisitorTeamName()}
            }},
            {"round", roundToString(match.GetRound())},
            {"tournamentId", match.TournamentId()}
        };

        if (!match.Id().empty()) {
            json["id"] = match.Id();
        }

        if (match.HasScore()) {
            json["score"] = match.MatchScore().value();
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Match>& match) {
        if (match) {
            to_json(json, *match);
        }
    }

    inline void from_json(const nlohmann::json& json, Match& match) {
        if (json.contains("id")) {
            match.SetId(json["id"].get<std::string>());
        }

        if (json.contains("tournamentId")) {
            match.SetTournamentId(json["tournamentId"].get<std::string>());
        }

        if (json.contains("home")) {
            auto home = json["home"];
            if (home.contains("id"))
                match.SetHomeTeamId(home["id"].get<std::string>());
            if (home.contains("name"))
                match.SetHomeTeamName(home["name"].get<std::string>());
        }

        if (json.contains("visitor")) {
            auto visitor = json["visitor"];
            if (visitor.contains("id"))
                match.SetVisitorTeamId(visitor["id"].get<std::string>());
            if (visitor.contains("name"))
                match.SetVisitorTeamName(visitor["name"].get<std::string>());
        }

        if (json.contains("round")) {
            match.SetRound(roundFromString(json["round"].get<std::string>()));
        }

        if (json.contains("score")) {
            Score score = json["score"].get<Score>();
            match.SetScore(score);
        }
    }

    inline void to_json(nlohmann::json& json, const std::vector<std::shared_ptr<Match>>& matches) {
        json = nlohmann::json::array();
        for (const auto& match : matches) {
            nlohmann::json matchJson;
            to_json(matchJson, match);
            json.push_back(matchJson);
        }
    }

}

#endif /* FC7CD637_41CC_48DE_8D8A_BC2CFC528D72 */
