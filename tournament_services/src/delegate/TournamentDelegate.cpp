//
// Created by tomas on 8/31/25.
//

#include <string>
#include <string_view>
#include <memory>
#include <utility>

#include "delegate/TournamentDelegate.hpp"

// ctor
TournamentDelegate::TournamentDelegate(
        std::shared_ptr<IRepository<domain::Tournament, std::string>> repository,
        std::shared_ptr<IQueueMessageProducer> producer)
    : tournamentRepository(std::move(repository)), producer(std::move(producer)) {}

// crea torneo y publica un mensaje
std::string TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    auto tp = std::move(tournament);
    std::string id = tournamentRepository->Create(*tp);

    // Enviar mensaje solo si hay id y si el producer existe
    if (!id.empty() && producer) {
        producer->SendMessage(std::string_view{id}, std::string_view{"tournament.created"});
    }

    return id;
}


// lee todos
std::vector<std::shared_ptr<domain::Tournament>> TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

// leer por id
std::shared_ptr<domain::Tournament> TournamentDelegate::ReadById(const std::string_view id) {
    return tournamentRepository->ReadById(std::string{id});
}

// actualizar: valida existencia, llama Update y considera exito si Update regresa id no vacio
bool TournamentDelegate::UpdateTournament(const domain::Tournament& t) {
    auto existing = tournamentRepository->ReadById(t.Id());
    if (!existing) return false;
    auto updatedId = tournamentRepository->Update(t);
    return !updatedId.empty();
}
