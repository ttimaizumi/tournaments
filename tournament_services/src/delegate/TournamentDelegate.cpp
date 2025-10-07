//
// Created by tomas on 8/31/25.
//
#include <string_view>
#include <memory>

#include "delegate/TournamentDelegate.hpp"

#include "persistence/repository/IRepository.hpp"

TournamentDelegate::TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string> > repository, std::shared_ptr<QueueMessageProducer> producer) : tournamentRepository(std::move(repository)), producer(std::move(producer)) {
}

std::string TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    //fill groups according to max groups
    std::shared_ptr<domain::Tournament> tp = std::move(tournament);
    // for (auto[i, g] = std::tuple{0, 'A'}; i < tp->Format().NumberOfGroups(); i++,g++) {
    //     tp->Groups().push_back(domain::Group{std::format("Tournament {}", g)});
    // }

    std::string id = tournamentRepository->Create(*tp);
    producer->SendMessage(id, "tournament.created");

    //if groups are completed also create matches

    return id;
}

std::vector<std::shared_ptr<domain::Tournament> > TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

std::shared_ptr<domain::Tournament> TournamentDelegate::ReadById(const std::string& id) {
    return tournamentRepository->ReadById(id);
}

void TournamentDelegate::Delete(const std::string& id) {
    tournamentRepository->Delete(id);
}