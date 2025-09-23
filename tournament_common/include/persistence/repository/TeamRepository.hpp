//
// Created by tomas on 8/24/25.
//

#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP
#include <string>


#include "IRepository.hpp"
#include "domain/Team.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"


class TeamRepository : public IRepository<domain::Team, std::string_view> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:

    explicit TeamRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider);

    std::vector<std::shared_ptr<domain::Team>> ReadAll() override;

    std::shared_ptr<domain::Team> ReadById(std::string_view id) override;

    std::string_view Create(const domain::Team &entity) override;

    std::string_view Update(const domain::Team &entity) override;

    void Delete(std::string_view id) override;
};


#endif //RESTAPI_TEAMREPOSITORY_HPP