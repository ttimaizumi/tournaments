//
// Created by edgar on 11/10/25.
//
#include "delegate/MatchDelegate.hpp"

using nlohmann::json;

MatchDelegate::MatchDelegate(std::shared_ptr<IMatchRepository> repo,
                             std::shared_ptr<IQueueMessageProducer> prod)
    : repository(std::move(repo)), producer(std::move(prod))
{
}

bool MatchDelegate::IsSingleElimination(const json& matchDoc)
{
    // Regla simple: todo lo que no sea "regular" lo tratamos como eliminatoria
    const auto round = matchDoc.value("round", std::string{"regular"});
    return round != "regular";
}

std::expected<std::vector<json>, std::string>
MatchDelegate::GetMatches(const std::string_view& tournamentId,
                          std::optional<std::string> showMatches)
{
    try
    {
        auto docs = repository->FindByTournament(tournamentId, showMatches);
        return docs;
    }
    catch (const std::exception& e)
    {
        return std::unexpected(std::string{"db-error: "} + e.what());
    }
}

std::expected<json, std::string>
MatchDelegate::GetMatch(const std::string_view& tournamentId,
                        const std::string_view& matchId)
{
    try
    {
        auto docOpt = repository->FindByTournamentAndId(tournamentId, matchId);
        if (!docOpt.has_value())
        {
            return std::unexpected(std::string{"match-not-found"});
        }
        return *docOpt;
    }
    catch (const std::exception& e)
    {
        return std::unexpected(std::string{"db-error: "} + e.what());
    }
}

std::expected<void, std::string>
MatchDelegate::UpdateScore(const std::string_view& tournamentId,
                           const std::string_view& matchId,
                           int homeScore,
                           int visitorScore)
{
    try
    {
        auto docOpt = repository->FindByTournamentAndId(tournamentId, matchId);
        if (!docOpt.has_value())
        {
            return std::unexpected(std::string{"match-not-found"});
        }

        auto& doc = *docOpt;

        // Regla de negocio: para eliminacion sencilla NO se permiten empates
        if (IsSingleElimination(doc) && homeScore == visitorScore)
        {
            return std::unexpected(std::string{"invalid-score"});
        }

        json scoreJson{
            {"home",    homeScore},
            {"visitor", visitorScore}
        };

        const bool updated = repository->UpdateScore(tournamentId, matchId, scoreJson, "played");
        if (!updated)
        {
            return std::unexpected(std::string{"update-failed"});
        }

        // Evento de dominio: se registro marcador
        if (producer)
        {
            producer->SendMessage(std::string{matchId}, std::string{"match.score-recorded"});
        }

        return {};
    }
    catch (const std::exception& e)
    {
        return std::unexpected(std::string{"db-error: "} + e.what());
    }
}
