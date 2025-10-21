//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <string>
#include <expected>

#include "cms/QueueMessageProducer.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"

class TournamentDelegate : public ITournamentDelegate{
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;
    std::shared_ptr<QueueMessageProducer> producer;
public:
    explicit TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository, std::shared_ptr<QueueMessageProducer> producer);

    std::expected<std::string, std::string> CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string> ReadAll() override;
    std::expected<std::shared_ptr<domain::Tournament>, std::string> ReadById(const std::string& id) override;
    std::expected<std::string, std::string> UpdateTournament(const domain::Tournament& tournament) override;
};

#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP