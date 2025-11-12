#include "delegate/MatchDelegate.hpp"
using nlohmann::json;

std::expected<json, std::string>
MatchDelegate::CreateMatch(const std::string_view& tournamentId,
                           const json& body) {
    try {
        // Validaciones basicas
        if (!body.contains("bracket") || !body["bracket"].is_string())
            return std::unexpected("invalid-body");
        std::string bracket = body["bracket"].get<std::string>();
        if (!isValidBracket(bracket))
            return std::unexpected("invalid-body");

        if (!body.contains("round") || !body["round"].is_number_integer())
            return std::unexpected("invalid-body");
        int round = body["round"].get<int>();
        if (round < 1) return std::unexpected("invalid-body");

        std::optional<std::string> homeId;
        std::optional<std::string> visitorId;
        if (body.contains("homeTeamId") && !body["homeTeamId"].is_null())
            homeId = body["homeTeamId"].get<std::string>();
        if (body.contains("visitorTeamId") && !body["visitorTeamId"].is_null())
            visitorId = body["visitorTeamId"].get<std::string>();

        json doc{
            {"tournamentId", std::string{tournamentId}},
            {"bracket", bracket},
            {"round", round},
            {"status", "scheduled"},
            {"score", json{{"home", 0}, {"visitor", 0}}}
        };
        if (homeId.has_value())   doc["homeTeamId"]    = homeId.value();
        if (visitorId.has_value())doc["visitorTeamId"] = visitorId.value();

        // advancement opcional
        if (body.contains("advancement") && body["advancement"].is_object()) {
            const auto& adv = body["advancement"];
            json advOut = json::object();
            if (adv.contains("winner") && adv["winner"].is_object()) {
                const auto& w = adv["winner"];
                if (!w.contains("matchId") || !w["matchId"].is_string()) return std::unexpected("invalid-body");
                if (!w.contains("slot") || !w["slot"].is_string()) return std::unexpected("invalid-body");
                if (!isValidSlot(w["slot"].get<std::string>())) return std::unexpected("invalid-body");
                advOut["winner"] = json{{"matchId", w["matchId"]}, {"slot", w["slot"]}};
            }
            if (adv.contains("loser") && adv["loser"].is_object()) {
                const auto& l = adv["loser"];
                if (!l.contains("matchId") || !l["matchId"].is_string()) return std::unexpected("invalid-body");
                if (!l.contains("slot") || !l["slot"].is_string()) return std::unexpected("invalid-body");
                if (!isValidSlot(l["slot"].get<std::string>())) return std::unexpected("invalid-body");
                advOut["loser"] = json{{"matchId", l["matchId"]}, {"slot", l["slot"]}};
            }
            if (!advOut.empty()) doc["advancement"] = advOut;
        }

        auto id = repository->Create(doc);
        if (!id.has_value()) return std::unexpected("db-error: insert-failed");

        doc["id"] = id.value();

        // Evento: match creado
        if (producer) producer->SendMessage(id.value(), "match.created");

        return doc;
    } catch (const std::exception& e) {
        return std::unexpected(std::string{"db-error: "} + e.what());
    }
}

std::expected<std::vector<json>, std::string>
MatchDelegate::GetMatches(const std::string_view& tournamentId,
                          std::optional<std::string> statusFilter) {
    try {
        auto v = repository->FindByTournament(tournamentId, statusFilter);
        return v;
    } catch (const std::exception& e) {
        return std::unexpected(std::string{"db-error: "} + e.what());
    }
}

std::expected<json, std::string>
MatchDelegate::GetMatch(const std::string_view& tournamentId,
                        const std::string_view& matchId) {
    try {
        auto r = repository->FindByTournamentAndId(tournamentId, matchId);
        if (!r.has_value()) return std::unexpected("not-found");
        return r.value();
    } catch (const std::exception& e) {
        return std::unexpected(std::string{"db-error: "} + e.what());
    }
}

std::expected<void, std::string>
MatchDelegate::UpdateScore(const std::string_view& tournamentId,
                           const std::string_view& matchId,
                           int homeScore,
                           int visitorScore) {
    try {
        if (homeScore == visitorScore) return std::unexpected("invalid-score");

        auto m = repository->FindByTournamentAndId(tournamentId, matchId);
        if (!m.has_value()) return std::unexpected("match-not-found");

        const json& md = m.value();
        if (!md.contains("homeTeamId") || !md.contains("visitorTeamId"))
            return std::unexpected("invalid-match");

        const std::string homeId = md["homeTeamId"].get<std::string>();
        const std::string visId  = md["visitorTeamId"].get<std::string>();

        json score{{"home", homeScore}, {"visitor", visitorScore}};
        const bool ok = repository->UpdateScore(tournamentId, matchId, score, "played");
        if (!ok) return std::unexpected("db-error: update-failed");

        // Avance basico (arbol fijo por referencias en el documento del match)
        if (md.contains("advancement") && md["advancement"].is_object()) {
            const bool homeWon = homeScore > visitorScore;
            const std::string winnerId = homeWon ? homeId : visId;
            const std::string loserId  = homeWon ? visId  : homeId;

            auto place = [&](const char* key, const std::string& teamId) {
                if (!md["advancement"].contains(key)) return;
                const auto& a = md["advancement"][key];
                if (!a.contains("matchId") || !a.contains("slot")) return;
                const std::string nextMatchId = a["matchId"].get<std::string>();
                const std::string slot = a["slot"].get<std::string>();
                if (slot == "home") {
                    repository->UpdateParticipants(tournamentId, nextMatchId, teamId, std::nullopt);
                } else if (slot == "visitor") {
                    repository->UpdateParticipants(tournamentId, nextMatchId, std::nullopt, teamId);
                }
                if (producer) producer->SendMessage(nextMatchId, "match.advanced");
            };

            place("winner", winnerId);
            place("loser", loserId);
        }

        // Evento: score registrado
        if (producer) producer->SendMessage(std::string{matchId}, "match.score-recorded");

        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string{"db-error: "} + e.what());
    }
}
