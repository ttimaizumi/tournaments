//
// Created by tomas on 8/31/25.
//
#include <string_view>
#include <memory>

#include "delegate/TournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"

TournamentDelegate::TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string> > repository, std::shared_ptr<QueueMessageProducer> producer) : tournamentRepository(std::move(repository)), producer(std::move(producer)) {
}

std::expected<std::string, std::string> TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    auto tournamentRepo = dynamic_cast<TournamentRepository*>(tournamentRepository.get());
    if(tournamentRepo && tournamentRepo->ExistsByName(tournament->Name())) {
        return std::unexpected("Tournament already exists");
    }

    // std::shared_ptr<domain::Tournament> tp = std::move(tournament);
    // for (auto[i, g] = std::tuple{0, 'A'}; i < tp->Format().NumberOfGroups(); i++,g++) {
    //     tp->Groups().push_back(domain::Group{std::format("Tournament {}", g)});
    // }

    try {
        std::string id = tournamentRepository->Create(*tournament);
        producer->SendMessage(id, "tournament.created");

        // if groups are completed also create matches

        return id;
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error creating tournament: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error creating tournament");
    }
}

std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string> TournamentDelegate::ReadAll() {
    try {
        return tournamentRepository->ReadAll();
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error reading tournaments: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error reading tournaments");
    }
}

std::expected<std::shared_ptr<domain::Tournament>, std::string> TournamentDelegate::ReadById(const std::string& id) {
    try {
        auto tournament = tournamentRepository->ReadById(id);
        if (tournament == nullptr) {
            return nullptr;
        }
        return tournament;
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error reading tournament: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error reading tournament");
    }
}

std::expected<std::string, std::string> TournamentDelegate::UpdateTournament(const domain::Tournament& tournament){
    auto tournamentRepo = dynamic_cast<TournamentRepository*>(tournamentRepository.get());
    if(tournamentRepo && !tournamentRepo->ExistsById(tournament.Id())) {
        return std::unexpected("Tournament not found");
    }

    try {
        return tournamentRepository->Update(tournament);
    } catch(const std::exception& e) {
        return std::unexpected(std::string("Error updating tournament: ") + e.what());
    } catch(...) {
        return std::unexpected("Unknown error updating tournament");
    }
}