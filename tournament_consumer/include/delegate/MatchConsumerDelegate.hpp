//MatchConsumerDelegate.hpp (consumer)
#pragma once
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <string_view>

#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"

#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"

#include "domain/Match.hpp"
#include "domain/WorldCupStrategy.hpp"

class MatchConsumerDelegate {
    std::shared_ptr<IMatchRepository>     matchRepository;
    std::shared_ptr<IGroupRepository>     groupRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;

    static std::string RoundKey(std::string_view r) {
        if (r == rounds::GROUP) return "group";
        if (r == rounds::R16)   return "r16";
        if (r == rounds::QF)    return "qf";
        if (r == rounds::SF)    return "sf";
        if (r == rounds::FINAL) return "final";
        return std::string(r);
    }

public:
    MatchConsumerDelegate(const std::shared_ptr<IMatchRepository>& matchRepository,
                  const std::shared_ptr<IGroupRepository>& groupRepository,
                  const std::shared_ptr<TournamentRepository>& tournamentRepository)
        : matchRepository(matchRepository),
          groupRepository(groupRepository),
          tournamentRepository(tournamentRepository) {}

    void ProcessTeamAddition(const TeamAddEvent& teamAddEvent) {
        std::cout << "[MatchDelegate/WC] Team added in tournament: "
                  << teamAddEvent.tournamentId << "\n";

        if (auto grp = groupRepository->FindByTournamentIdAndGroupId(
                teamAddEvent.tournamentId, teamAddEvent.groupId)) {
            auto tRes = tournamentRepository->ReadById(teamAddEvent.tournamentId);
            int expectedPerGroup = tRes ? tRes->Format().MaxTeamsPerGroup() : 0;
            std::cout << "[WC] Group " << grp->Name() << ": "
                      << grp->Teams().size() << "/" << expectedPerGroup
                      << " teams\n";
        }

        {
            auto allGroups = groupRepository->FindByTournamentId(teamAddEvent.tournamentId);
            if (!allGroups.empty()) {
                auto tRes = tournamentRepository->ReadById(teamAddEvent.tournamentId);
                int expectedGroups   = tRes ? tRes->Format().NumberOfGroups()   : 0;
                int expectedPerGroup = tRes ? tRes->Format().MaxTeamsPerGroup() : 0;

                int complete = 0;
                for (const auto& g : allGroups) {
                    if (static_cast<int>(g->Teams().size()) == expectedPerGroup) complete++;
                }
                std::cout << "[WC] Progress: " << complete << "/" << expectedGroups
                          << " groups complete\n";
            }
        }

        if (IsTournamentReady(teamAddEvent.tournamentId)) {
            std::cout << "[MatchDelegate/WC] All groups complete -> creating group-stage matches...\n";
            CreateGroupStageMatches(teamAddEvent.tournamentId);
        } else {
            std::cout << "[MatchDelegate/WC] Tournament not ready yet; waiting for more teams...\n";
        }
    }

    void ProcessScoreUpdate(const ScoreUpdateEvent& e) {
        std::cout << "[MatchDelegate/WC] Score update for tournament: " << e.tournamentId << "\n";

        if (!AllGroupMatchesPlayed(e.tournamentId)) {
            std::cout << "[MatchDelegate/WC] Still pending group matches...\n";
            return;
        }

        CreateKnockoutMatches(e.tournamentId);
    }

private:
    bool IsTournamentReady(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) {
            std::cout << "[WC] Tournament not found\n";
            return false;
        }

        auto groups = groupRepository->FindByTournamentId(tournamentId);
        const int expectedGroups = t->Format().NumberOfGroups();
        if (static_cast<int>(groups.size()) != expectedGroups) {
            std::cout << "[WC] Groups count mismatch. Have " << groups.size()
                      << " expected " << expectedGroups << "\n";
            return false;
        }

        const int teamsPerGroup = t->Format().MaxTeamsPerGroup();
        for (const auto& g : groups) {
            if (static_cast<int>(g->Teams().size()) != teamsPerGroup) {
                std::cout << "[WC] Group " << g->Name() << " has "
                          << g->Teams().size() << " teams (expected "
                          << teamsPerGroup << ")\n";
                return false;
            }
        }
        return true;
    }

    // FIX: ignore string status; rely only on score presence
    bool AllGroupMatchesPlayed(const std::string& tournamentId) {
        auto all = matchRepository->FindByTournamentId(tournamentId);
        for (const auto& m : all) {
            if (!m) continue;
            if (m->Round() == rounds::GROUP && !m->HasScore())
                return false;
        }
        return true;
    }

    void CreateGroupStageMatches(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) {
            std::cout << "[WC] ERROR: tournament not found\n";
            return;
        }

        auto groups = groupRepository->FindByTournamentId(tournamentId);

        WorldCupStrategy s;
        auto createdOrErr = s.CreateRegularPhaseMatches(*t, groups);
        if (!createdOrErr) {
            std::cout << "[WC] Strategy error: " << createdOrErr.error() << "\n";
            return;
        }
        const auto& created = createdOrErr.value();

        int ok = 0;
        for (const auto& m : created) {
            const std::string id = matchRepository->Create(m);
            if (!id.empty()) ok++;
            else std::cout << "[WC] ERROR creating match\n";
        }
        std::cout << "[WC] Created " << ok << "/" << created.size()
                  << " group matches\n";
    }

    void CreateKnockoutMatches(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) {
            std::cout << "[WC] ERROR: tournament not found\n";
            return;
        }

        auto groups = groupRepository->FindByTournamentId(tournamentId);
        auto all    = matchRepository->FindByTournamentId(tournamentId);

        auto makeKey = [&](const domain::Match& m) {
            const std::string r   = RoundKey(m.Round());
            const std::string hid = m.Home().Id();
            const std::string vid = m.Visitor().Id();
            return r + "|" + hid + "|" + vid;
        };

        std::unordered_set<std::string> existing;
        existing.reserve(all.size());
        for (const auto& sp : all) {
            if (!sp) continue;
            const auto& m = *sp;
            if (m.Round() == rounds::R16 || m.Round() == rounds::QF ||
                m.Round() == rounds::SF  || m.Round() == rounds::FINAL) {
                const auto& hid = m.Home().Id();
                const auto& vid = m.Visitor().Id();
                if (!hid.empty() && !vid.empty()) {
                    existing.insert(makeKey(m));
                }
            }
        }

        WorldCupStrategy s;
        auto createdOrErr = s.CreatePlayoffMatches(*t, all, groups);
        if (!createdOrErr) {
            std::cout << "[WC] Strategy error: " << createdOrErr.error() << "\n";
            return;
        }
        const auto& proposals = createdOrErr.value();

        std::vector<domain::Match> toCreate;
        toCreate.reserve(proposals.size());
        for (const auto& m : proposals) {
            const auto& hid = m.Home().Id();
            const auto& vid = m.Visitor().Id();
            if (hid.empty() || vid.empty()) continue;
            if (existing.find(makeKey(m)) != existing.end()) continue;
            toCreate.push_back(m);
        }

        if (toCreate.empty()) {
            std::cout << "[WC] KO bracket already up-to-date (or waiting for more results)\n";
            return;
        }

        int inserted = 0, skipped = 0;
        for (const auto& m : toCreate) {
            // Prefer idempotent insert if your repo lo soporta; si no, usa Create(m)
            std::string id;
            try {
                id = matchRepository->CreateIfNotExists(m);
            } catch (...) {
                id.clear();
            }
            if (id.empty()) {
                try {
                    id = matchRepository->Create(m);
                } catch (...) {
                    id.clear();
                }
            }
            if (!id.empty()) inserted++; else skipped++;
        }

        std::cout << "[WC] Created " << inserted << " knockout matches; " << skipped << " already existed\n";
    }
};
