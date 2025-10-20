#ifndef SUPPORT_ITOURNAMENTDELEGATE_HPP
#define SUPPORT_ITOURNAMENTDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "model/Tournament.hpp"

// Stub de interfaz para tests
class ITournamentDelegate {
public:
    virtual ~ITournamentDelegate() = default;
    virtual std::expected<std::string, int> Create(const Tournament& tournament) = 0;
    virtual std::optional<Tournament> FindById(const std::string& id) = 0;
    virtual std::vector<Tournament> List() = 0;
    virtual std::expected<void, int> Update(const std::string& id, const Tournament& tournament) = 0;
};

#endif // SUPPORT_ITOURNAMENTDELEGATE_HPP
