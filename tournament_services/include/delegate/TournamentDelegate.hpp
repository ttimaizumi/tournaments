//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <string>

#include "cms/QueueMessageProducer.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"

class TournamentDelegate : public ITournamentDelegate{
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;
    std::shared_ptr<QueueMessageProducer> producer;
public:
    explicit TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository, std::shared_ptr<QueueMessageProducer> producer);

    std::shared_ptr<domain::Tournament> GetTournament(std::string_view id) override;
    std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;
    std::string UpdateTournament(const domain::Tournament& tournament) override;
};

#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP