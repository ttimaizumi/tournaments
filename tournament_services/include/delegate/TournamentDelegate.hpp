//
// Created by tsuny on 8/31/25.
//
#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <memory>
#include <string>
#include <vector>

#include "cms/QueueMessageProducer.hpp"          // <-- correct path under cms/
#include "delegate/ITournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"

class TournamentDelegate : public ITournamentDelegate {
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;
    std::shared_ptr<QueueMessageProducer> producer;

public:
    explicit TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository,
                                std::shared_ptr<QueueMessageProducer> producer);

    std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;

    std::shared_ptr<domain::Tournament> ReadById(const std::string& id) override;
    bool UpdateTournament(const std::string& id, const domain::Tournament& tournament) override;
    bool DeleteTournament(const std::string& id) override;
};

#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP
