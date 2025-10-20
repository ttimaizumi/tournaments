#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "domain/Tournament.hpp"
#include "exception/Error.hpp"
#include "persistence/repository/IRepository.hpp"
#include "delegate/ITournamentDelegate.hpp"

class TournamentDelegate : public ITournamentDelegate {
public:
    TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository);

    std::expected<std::vector<std::shared_ptr<domain::Tournament>>, Error> ReadAll() override;
    std::expected<std::shared_ptr<domain::Tournament>, Error> GetTournament(std::string_view id) override;
    std::expected<std::string, Error> CreateTournament(const domain::Tournament& tournament) override;
    std::expected<std::string, Error> UpdateTournament(const domain::Tournament& tournament) override;
    std::expected<void, Error> DeleteTournament(std::string_view id) override;

private:
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;
};


#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP