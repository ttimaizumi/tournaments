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
        if (type == "DOUBLE_ELIMINATION")
            return TournamentType::DOUBLE_ELIMINATION;

        return TournamentType::DOUBLE_ELIMINATION;
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
            case TournamentType::DOUBLE_ELIMINATION:
                json["type"] = "DOUBLE_ELIMINATION";
                break;
            default:
                json["type"] = "DOUBLE_ELIMINATION";
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
        if(json.contains("teams") && json["teams"].is_array()) {
            json["teams"].get_to(group.Teams());
        }
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

    inline std::string bracketTypeToString(BracketType type) {
        switch (type) {
            case BracketType::WINNERS: return "WINNERS";
            case BracketType::LOSERS: return "LOSERS";
            case BracketType::FINAL: return "FINAL";
            default: return "WINNERS";
        }
    }

    inline BracketType bracketTypeFromString(std::string_view type) {
        if (type == "WINNERS") return BracketType::WINNERS;
        if (type == "LOSERS") return BracketType::LOSERS;
        if (type == "FINAL") return BracketType::FINAL;
        return BracketType::WINNERS;
    }

    inline void to_json(nlohmann::json& json, const Score& score) {
        json = {
            {"homeTeamScore", score.homeTeamScore},
            {"visitorTeamScore", score.visitorTeamScore}
        };
    }

    inline void from_json(const nlohmann::json& json, Score& score) {
        if (json.contains("homeTeamScore"))
            json.at("homeTeamScore").get_to(score.homeTeamScore);
        if (json.contains("visitorTeamScore"))
            json.at("visitorTeamScore").get_to(score.visitorTeamScore);
    }

    inline void to_json(nlohmann::json& json, const Match& match) {
        json = nlohmann::json::object();
        if (!match.Id().empty()) {
            json["id"] = match.Id();
        }
        if (!match.TournamentId().empty()) {
            json["tournamentId"] = match.TournamentId();
        }
        if (!match.HomeTeamId().empty()) {
            json["homeTeamId"] = match.HomeTeamId();
        }
        if (!match.VisitorTeamId().empty()) {
            json["visitorTeamId"] = match.VisitorTeamId();
        }
        json["score"] = match.MatchScore();
        if (!match.WinnerNextMatchId().empty()) {
            json["winnerNextMatchId"] = match.WinnerNextMatchId();
        }
        if (!match.LoserNextMatchId().empty()) {
            json["loserNextMatchId"] = match.LoserNextMatchId();
        }
        json["roundNumber"] = match.RoundNumber();
        json["bracketType"] = bracketTypeToString(match.Bracket());
        json["isFirstFinal"] = match.IsFirstFinal();
    }

    inline void from_json(const nlohmann::json& json, Match& match) {
        if (json.contains("id")) {
            match.Id() = json["id"].get<std::string>();
        }
        if (json.contains("tournamentId")) {
            match.TournamentId() = json["tournamentId"].get<std::string>();
        }
        if (json.contains("homeTeamId")) {
            match.HomeTeamId() = json["homeTeamId"].get<std::string>();
        }
        if (json.contains("visitorTeamId")) {
            match.VisitorTeamId() = json["visitorTeamId"].get<std::string>();
        }
        if (json.contains("score")) {
            json.at("score").get_to(match.MatchScore());
        }
        if (json.contains("winnerNextMatchId")) {
            match.WinnerNextMatchId() = json["winnerNextMatchId"].get<std::string>();
        }
        if (json.contains("loserNextMatchId")) {
            match.LoserNextMatchId() = json["loserNextMatchId"].get<std::string>();
        }
        if (json.contains("roundNumber")) {
            json.at("roundNumber").get_to(match.RoundNumber());
        }
        if (json.contains("bracketType")) {
            match.Bracket() = bracketTypeFromString(json["bracketType"].get<std::string>());
        }
        if (json.contains("isFirstFinal")) {
            json.at("isFirstFinal").get_to(match.IsFirstFinal());
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Match>& match) {
        json = nlohmann::json::object();
        if (!match->Id().empty()) {
            json["id"] = match->Id();
        }
        if (!match->TournamentId().empty()) {
            json["tournamentId"] = match->TournamentId();
        }
        if (!match->HomeTeamId().empty()) {
            json["homeTeamId"] = match->HomeTeamId();
        }
        if (!match->VisitorTeamId().empty()) {
            json["visitorTeamId"] = match->VisitorTeamId();
        }
        json["score"] = match->MatchScore();
        if (!match->WinnerNextMatchId().empty()) {
            json["winnerNextMatchId"] = match->WinnerNextMatchId();
        }
        if (!match->LoserNextMatchId().empty()) {
            json["loserNextMatchId"] = match->LoserNextMatchId();
        }
        json["roundNumber"] = match->RoundNumber();
        json["bracketType"] = bracketTypeToString(match->Bracket());
        json["isFirstFinal"] = match->IsFirstFinal();
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
