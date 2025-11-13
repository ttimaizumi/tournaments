//
// Created by edgar on 11/12/25.
//

#include "cms/TournamentFullListener.hpp"

#include <nlohmann/json.hpp>
#include <iostream>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <vector>

using nlohmann::json;

namespace {
    struct Competitor {
        std::string id;
        std::string name;
    };
}

TournamentFullListener::TournamentFullListener(
    const std::shared_ptr<ConnectionManager>& cm,
    const std::shared_ptr<IMatchRepository>& matchRepository,
    const std::shared_ptr<IGroupRepository>& groupRepository)
    : QueueMessageListener(cm),
      matchRepo(matchRepository),
      groupRepo(groupRepository) {}

void TournamentFullListener::processMessage(const std::string& message) {
    std::cout << "[TournamentFullListener] Mensaje recibido: " << message << std::endl;

    try {
        json evt = json::parse(message);

        if (!evt.contains("tournamentId") || !evt.contains("groupId")) {
            std::cout << "[TournamentFullListener][ERROR] Falta tournamentId o groupId en el evento" << std::endl;
            return;
        }

        const std::string tournamentId = evt.at("tournamentId").get<std::string>();
        const std::string groupId      = evt.at("groupId").get<std::string>();

        auto group = groupRepo->FindByTournamentIdAndGroupId(tournamentId, groupId);
        if (!group) {
            std::cout << "[TournamentFullListener][ERROR] No se encontro el grupo para torneo="
                      << tournamentId << " groupId=" << groupId << std::endl;
            return;
        }

        const auto& teams = group->Teams();
        std::cout << "[TournamentFullListener] El grupo tiene " << teams.size() << " equipos" << std::endl;

        if (teams.size() != 32) {
            std::cout << "[TournamentFullListener][ERROR] Se esperaban 32 equipos, llegaron "
                      << teams.size() << std::endl;
            return;
        }

        // Copiamos los equipos a una estructura interna mas simple
        std::vector<Competitor> allCompetitors;
        allCompetitors.reserve(teams.size());
        for (const auto& t : teams) {
            allCompetitors.push_back(Competitor{t.Id, t.Name});
        }

        // Barajamos SOLO una vez al inicio: aqui se fija la rama del torneo
        std::mt19937 rng{std::random_device{}()};
        std::shuffle(allCompetitors.begin(), allCompetitors.end(), rng);

        // Mapa de perdidas por equipo (para doble eliminacion)
        std::unordered_map<std::string, int> losses;
        for (const auto& c : allCompetitors) {
            losses[c.id] = 0;
        }

        // Brackets
        std::vector<Competitor> winnersBracket = allCompetitors; // todos empiezan en winners
        std::vector<Competitor> losersBracket;
        std::vector<Competitor> eliminated;
        std::vector<Competitor> pendingToLosers;

        // Helper: crea un match, asigna marcador random (sin empate) y regresa ganador / perdedor
        auto createMatchAndPlay =
            [&](const std::string& bracket,
                int round,
                const Competitor& home,
                const Competitor& visitor) -> std::pair<Competitor, Competitor>
        {
            json matchDoc;
            matchDoc["tournamentId"]    = tournamentId;
            matchDoc["groupId"]         = groupId;
            matchDoc["round"]           = round;
            matchDoc["bracket"]         = bracket;
            matchDoc["status"]          = "scheduled";
            matchDoc["homeTeamId"]      = home.id;
            matchDoc["homeTeamName"]    = home.name;
            matchDoc["visitorTeamId"]   = visitor.id;
            matchDoc["visitorTeamName"] = visitor.name;
            matchDoc["score"]           = json{{"home", 0}, {"visitor", 0}};

            auto matchIdOpt = matchRepo->Create(matchDoc);

            // Simulamos marcador
            std::uniform_int_distribution<int> dist(0, 5);
            int hs = 0;
            int vs = 0;
            do {
                hs = dist(rng);
                vs = dist(rng);
            } while (hs == vs); // sin empate

            json newScore = json{{"home", hs}, {"visitor", vs}};

            if (matchIdOpt) {
                const std::string& matchId = *matchIdOpt; // SIN .value()
                bool updated = matchRepo->UpdateScore(tournamentId, matchId, newScore, "played");
                if (!updated) {
                    std::cout << "[TournamentFullListener][WARN] No se pudo actualizar el score para match "
                              << matchId << std::endl;
                } else {
                    std::cout << "[TournamentFullListener] "
                              << bracket << " R" << round << " - "
                              << home.name << " vs " << visitor.name
                              << " => " << hs << "-" << vs
                              << " (matchId=" << matchId << ")"
                              << std::endl;
                }
            } else {
                std::cout << "[TournamentFullListener][ERROR] Create() regreso optional vacio" << std::endl;
                std::cout << "[TournamentFullListener] Aun asi continuo con la simulacion en memoria." << std::endl;
            }

            bool homeWon = hs > vs;
            Competitor winner = homeWon ? home : visitor;
            Competitor loser  = homeWon ? visitor : home;

            losses[loser.id] += 1;

            std::cout << "[TournamentFullListener] Resultado interno: "
                      << winner.name << " gana, "
                      << loser.name  << " acumula " << losses[loser.id] << " perdidas"
                      << std::endl;

            return {winner, loser};
        };

        auto playWinnersRound = [&](int round) {
            std::vector<Competitor> nextWinners;
            pendingToLosers.clear();

            // NO volvemos a barajar: la rama queda determinada por el orden actual
            for (std::size_t i = 0; i + 1 < winnersBracket.size(); i += 2) {
                const auto& home    = winnersBracket[i];
                const auto& visitor = winnersBracket[i + 1];

                auto [winner, loser] = createMatchAndPlay("winners", round, home, visitor);

                nextWinners.push_back(winner);      // sigue en winners
                pendingToLosers.push_back(loser);   // baja a losers (1 perdida)
            }

            // Si hay numero impar, el ultimo pasa bye en winners
            if (winnersBracket.size() % 2 == 1) {
                nextWinners.push_back(winnersBracket.back());
            }

            winnersBracket = std::move(nextWinners);
        };

        auto playLosersRound = [&](int round) {
            if (losersBracket.size() < 2) {
                return;
            }

            std::vector<Competitor> nextLosers;

            // Sin barajar: el orden actual define la rama
            for (std::size_t i = 0; i + 1 < losersBracket.size(); i += 2) {
                const auto& home    = losersBracket[i];
                const auto& visitor = losersBracket[i + 1];

                auto [winner, loser] = createMatchAndPlay("losers", round, home, visitor);

                if (losses[loser.id] >= 2) {
                    eliminated.push_back(loser); // fuera del torneo
                } else {
                    // En teoria no deberia pasar (en losers ya traen 1 perdida),
                    // pero si pasa, lo dejamos vivo
                    nextLosers.push_back(loser);
                }

                // Ganador se mantiene vivo en losers
                nextLosers.push_back(winner);
            }

            // Bye si hay impar
            if (losersBracket.size() % 2 == 1) {
                nextLosers.push_back(losersBracket.back());
            }

            losersBracket = std::move(nextLosers);
        };

        int winnersRound = 1;
        int losersRound  = 1;

        // Jugar winners + losers hasta que solo quede 1 en winners
        while (winnersBracket.size() > 1) {
            std::cout << "[TournamentFullListener] Winners R" << winnersRound
                      << " con " << winnersBracket.size() << " equipos" << std::endl;

            playWinnersRound(winnersRound);

            // Los perdedores de winners pasan a losers con 1 perdida
            losersBracket.insert(losersBracket.end(),
                                 pendingToLosers.begin(),
                                 pendingToLosers.end());
            pendingToLosers.clear();

            std::cout << "[TournamentFullListener] Despues de winners R" << winnersRound
                      << ": winners=" << winnersBracket.size()
                      << ", losers=" << losersBracket.size()
                      << ", eliminados=" << eliminated.size()
                      << std::endl;

            if (losersBracket.size() > 1) {
                std::cout << "[TournamentFullListener] Losers R" << losersRound
                          << " con " << losersBracket.size() << " equipos" << std::endl;
                playLosersRound(losersRound);
                std::cout << "[TournamentFullListener] Despues de losers R" << losersRound
                          << ": losers=" << losersBracket.size()
                          << ", eliminados=" << eliminated.size()
                          << std::endl;
                ++losersRound;
            }

            ++winnersRound;
        }

        // Campeon de winners
        Competitor winnersChampion = winnersBracket.front();
        std::cout << "[TournamentFullListener] Campeon del winners bracket: "
                  << winnersChampion.name << std::endl;

        // Reducir losers hasta dejar uno
        while (losersBracket.size() > 1) {
            std::cout << "[TournamentFullListener] Losers extra R" << losersRound
                      << " con " << losersBracket.size() << " equipos" << std::endl;
            playLosersRound(losersRound);
            std::cout << "[TournamentFullListener] Despues de losers extra R" << losersRound
                      << ": losers=" << losersBracket.size()
                      << ", eliminados=" << eliminated.size()
                      << std::endl;
            ++losersRound;
        }

        if (losersBracket.empty()) {
            std::cout << "[TournamentFullListener] No quedo campeon de losers, torneo termina como single elimination."
                      << std::endl;
            std::cout << "[TournamentFullListener] Campeon absoluto: "
                      << winnersChampion.name << std::endl;
            return;
        }

        Competitor losersChampion = losersBracket.front();
        std::cout << "[TournamentFullListener] Campeon del losers bracket: "
                  << losersChampion.name << std::endl;

        // Gran final de doble eliminacion
        int finalRound = std::max(winnersRound, losersRound);

        auto [finalWinner1, finalLoser1] =
            createMatchAndPlay("final", finalRound, winnersChampion, losersChampion);

        if (finalWinner1.id == winnersChampion.id) {
            std::cout << "[TournamentFullListener] Final decidida en un partido. Campeon absoluto: "
                      << finalWinner1.name << std::endl;
            return;
        }

        // Si gano el campeon de losers, hay reset (segunda final)
        std::cout << "[TournamentFullListener] Reset final (ambos con una perdida). "
                  << "Jugando segunda final..." << std::endl;

        auto [finalWinner2, finalLoser2] =
            createMatchAndPlay("final", finalRound + 1, winnersChampion, losersChampion);

        std::cout << "[TournamentFullListener] Campeon absoluto despues del reset: "
                  << finalWinner2.name << std::endl;

    } catch (const std::exception& ex) {
        std::cout << "[TournamentFullListener][ERROR] Excepcion: " << ex.what() << std::endl;
    }
}
