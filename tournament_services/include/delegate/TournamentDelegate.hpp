//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <memory>
#include <string>

#include "domain/Tournament.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "delegate/ITournamentDelegate.hpp"

class TournamentDelegate : public ITournamentDelegate {
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;
    std::shared_ptr<IQueueMessageProducer> producer;

public:
    // constructor usando la interfaz del productor para facilitar tests
    explicit TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository,
                                std::shared_ptr<IQueueMessageProducer> producer);

    // crea torneo y publica evento
    std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;

    // lee todos
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;

    // nuevos metodos para cubrir tests del controller
    std::shared_ptr<domain::Tournament> ReadById(const std::string_view id) override;
    bool UpdateTournament(const domain::Tournament& t) override;
};

#endif // TOURNAMENTS_TOURNAMENTDELEGATE_HPP
