//
// Created by tomas on 8/31/25.
//
#include <string_view>
#include <memory>

#include "delegate/TournamentDelegate.hpp"

#include "persistence/repository/IRepository.hpp"
#include "domain/Utilities.hpp"

TournamentDelegate::TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string> > repository, std::shared_ptr<QueueMessageProducer> producer) : tournamentRepository(std::move(repository)), producer(std::move(producer)) {
}

std::shared_ptr<domain::Tournament> TournamentDelegate::GetTournament(std::string_view id) {
    return tournamentRepository->ReadById(id.data());
}

std::string TournamentDelegate::CreateTournament(const domain::Tournament& tournament) {
    //fill groups according to max groups
    domain::Tournament tp = tournament;
    // for (auto[i, g] = std::tuple{0, 'A'}; i < tp->Format().NumberOfGroups(); i++,g++) {
    //     tp->Groups().push_back(domain::Group{std::format("Tournament {}", g)});
    // }

    std::string id = tournamentRepository->Create(tp);
    producer->SendMessage(id, "tournament.created");

    //if groups are completed also create matches

    return id;
}

std::string TournamentDelegate::UpdateTournament(const domain::Tournament& tournament) {
    return tournamentRepository->Update(tournament);
}

std::vector<std::shared_ptr<domain::Tournament> > TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

void TournamentDelegate::DeleteTournament(std::string_view id) {
    tournamentRepository->Delete(id.data());
}