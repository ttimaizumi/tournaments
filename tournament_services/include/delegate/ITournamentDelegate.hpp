//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_ITOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_ITOURNAMENTDELEGATE_HPP

#include <string>
#include <memory>
#include <string_view>
#include <vector>
#include "domain/Tournament.hpp"

class ITournamentDelegate {
public:
    virtual ~ITournamentDelegate() = default;

    // crear torneo -> regresa id (string). si falla, regresa string vacio
    virtual std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) = 0;

    // leer por id
    virtual std::shared_ptr<domain::Tournament> ReadById(std::string_view id) = 0;

    // leer todos
    virtual std::vector<std::shared_ptr<domain::Tournament>> ReadAll() = 0;

    // actualizar -> true si actualizo, false si no existe
    virtual bool UpdateTournament(const domain::Tournament& t) = 0;
};

#endif //TOURNAMENTS_ITOURNAMENTDELEGATE_HPP