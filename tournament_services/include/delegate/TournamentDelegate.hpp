// TournamentDelegate.hpp
#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <memory>
#include <string>
#include <expected> // necesario para std::expected

#include "domain/Tournament.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "delegate/ITournamentDelegate.hpp"

class TournamentDelegate : public ITournamentDelegate {
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;
    std::shared_ptr<IQueueMessageProducer> producer;

public:
    explicit TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository);
    TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository,
                       std::shared_ptr<IQueueMessageProducer> producer);

    // existentes
    std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;
    std::shared_ptr<domain::Tournament> ReadById(const std::string_view id) override;
    bool UpdateTournament(const domain::Tournament& t) override;

    // nuevos (para pruebas con std::expected segun requerimientos)
    // Crea y retorna id o error "conflict"
    std::expected<std::string, std::string>
    CreateTournamentEx(std::shared_ptr<domain::Tournament> tournament);

    // Actualiza si existe; si no existe -> error "not found".
    // Firma toma valores explicitos (id y name) para simplificar el ejemplo de prueba.
    std::expected<void, std::string>
    UpdateTournamentEx(const domain::Tournament& base, const std::string& id, const std::string& newName);
};

#endif // TOURNAMENTS_TOURNAMENTDELEGATE_HPP
