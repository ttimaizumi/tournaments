//
// Created by tsuny on 9/1/25.
//

#ifndef TOURNAMENTS_TOURNAMENTREPOSITORY_HPP
#define TOURNAMENTS_TOURNAMENTREPOSITORY_HPP
#include <string>

#include "IRepository.hpp"
#include "domain/Tournament.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"


class TournamentRepository : public IRepository<domain::Tournament, std::string> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit TournamentRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider);
    std::shared_ptr<domain::Tournament> ReadById(std::string id) override;
    std::string Create (const domain::Tournament & entity) override;
    std::string Update (const domain::Tournament & entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;

    bool ExistsByName(const std::string& name);
};

#endif //TOURNAMENTS_TOURNAMENTREPOSITORY_HPP