#ifndef SUPPORT_TOURNAMENTDELEGATE_HPP
#define SUPPORT_TOURNAMENTDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "model/Tournament.hpp"

// Stub de TournamentDelegate para tests
template<typename TournamentRepositoryMock>
class TournamentDelegate {
public:
    explicit TournamentDelegate(TournamentRepositoryMock& repository) : repository_(repository) {}

    std::expected<std::string, int> Create(const Tournament& tournament) {
        auto result = repository_.Insert(tournament);
        if (result) {
            return *result;
        }
        return std::unexpected{409};
    }

    std::optional<Tournament> FindById(const std::string& id) {
        return repository_.FindById(id);
    }

    std::vector<Tournament> List() {
        return repository_.List();
    }

    std::expected<void, int> Update(const std::string& id, const Tournament& tournament) {
        if (repository_.Update(id, tournament)) {
            return {};
        }
        return std::unexpected{404};
    }

private:
    TournamentRepositoryMock& repository_;
};

#endif // SUPPORT_TOURNAMENTDELEGATE_HPP
