// TeamController.cpp
//
// Created by root on 9/27/25.
//


#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate)
    : teamDelegate(teamDelegate) {}

// GET /teams/<id>
// - valida formato de id
// - llama al delegate
// - 200 con JSON si existe, 404 si no existe
crow::response TeamController::getTeam(const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if (auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
        nlohmann::json body = team;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::NOT_FOUND, "team not found"};
}

// GET /teams
// - lista todos los equipos
// - 200 con arreglo JSON (vacio o con elementos)
crow::response TeamController::getAllTeams() const {
    nlohmann::json body = teamDelegate->GetAllTeams();
    crow::response response{200, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

// POST /teams
// - transforma JSON -> domain::Team
// - delega en SaveTeam
// - 201 con header Location si crea
// - 409 si el delegate regresa id vacio (conflicto/insercion fallida)
// - 400 si el cuerpo no es JSON valido
crow::response TeamController::SaveTeam(const crow::request& request) const {
    crow::response response;

    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Team team = requestBody;

    auto createdId = teamDelegate->SaveTeam(team);

    if (createdId.empty()) {
        response.code = crow::CONFLICT;
        return response;
    }

    response.code = crow::CREATED;
    response.add_header("location", createdId.data());
    return response;
}

// PUT /teams
// - transforma JSON -> domain::Team
// - delega en UpdateTeam
// - 204 si actualizo, 404 si no encontro el ID
// - 400 si el cuerpo no es JSON valido
crow::response TeamController::UpdateTeam(const crow::request& request) const {
    crow::response response;

    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    auto body = nlohmann::json::parse(request.body);
    domain::Team team = body;

    const bool updated = teamDelegate->UpdateTeam(team);
    response.code = updated ? crow::NO_CONTENT : crow::NOT_FOUND;
    return response;
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams", "PUT"_method)
