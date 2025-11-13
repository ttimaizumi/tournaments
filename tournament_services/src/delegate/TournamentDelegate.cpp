// TournamentDelegate.cpp
#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <expected>

#include "delegate/TournamentDelegate.hpp"

TournamentDelegate::TournamentDelegate(
    std::shared_ptr<IRepository<domain::Tournament, std::string>> repository)
    : tournamentRepository(std::move(repository)), producer(nullptr) {}

TournamentDelegate::TournamentDelegate(
    std::shared_ptr<IRepository<domain::Tournament, std::string>> repository,
    std::shared_ptr<IQueueMessageProducer> producerIn)
    : tournamentRepository(std::move(repository)), producer(std::move(producerIn)) {}

std::string TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    auto tp = std::move(tournament);
    std::string id = tournamentRepository->Create(*tp);

    if (!id.empty() && producer) {
        producer->SendMessage(id, "tournament.created");
    }
    return id;
}

std::expected<std::string, std::string>
TournamentDelegate::CreateTournamentEx(std::shared_ptr<domain::Tournament> tournament) {
    auto tp = std::move(tournament);
    std::string id = tournamentRepository->Create(*tp);
    if (id.empty()) {
        return std::unexpected(std::string{"conflict"});
    }
    if (producer) {
        producer->SendMessage(id, "tournament.created");
    }
    return id;
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

std::shared_ptr<domain::Tournament> TournamentDelegate::ReadById(const std::string_view id) {
    return tournamentRepository->ReadById(std::string{id});
}

bool TournamentDelegate::UpdateTournament(const domain::Tournament& t) {
    auto existing = tournamentRepository->ReadById(t.Id());
    if (!existing) return false;
    auto updatedId = tournamentRepository->Update(t);
    return !updatedId.empty();
}

std::expected<void, std::string>
TournamentDelegate::UpdateTournamentEx(const domain::Tournament& base,
                                       const std::string& id,
                                       const std::string& newName) {
    auto existing = tournamentRepository->ReadById(id);
    if (!existing) {
        return std::unexpected(std::string{"not found"});
    }
    domain::Tournament t = base;
    t.Id() = id;
    t.Name() = newName;

    auto updatedId = tournamentRepository->Update(t);
    if (updatedId.empty()) {
        return std::unexpected(std::string{"update failed"});
    }
    return {};
}