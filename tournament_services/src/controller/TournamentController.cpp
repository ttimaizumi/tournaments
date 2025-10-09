// TournamentController.cpp
#include <nlohmann/json.hpp>
#include <regex>

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"   // trae to_json/from_json para Tournament y Format

using nlohmann::json;

// Regex local para validar IDs: letras, digitos, _ o -, al menos 1 char
static const std::regex kIdPattern{R"(^[A-Za-z0-9_-]+$)"};

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate)
    : tournamentDelegate(std::move(delegate)) {}

/// POST /tournaments
crow::response TournamentController::CreateTournament(const crow::request& request) const {
    crow::response response;

    // 1) JSON sintactico valido
    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        response.body = "invalid json";
        return response;
    }

    // 2) Validacion de esquema minimo: name requerido y string
    nlohmann::json body = nlohmann::json::parse(request.body);
    if (!body.contains("name") || !body["name"].is_string()) {
        response.code = crow::BAD_REQUEST;
        response.body = "field 'name' is required and must be string";
        return response;
    }
    if (body.contains("format") && !body["format"].is_object()) {
        response.code = crow::BAD_REQUEST;
        response.body = "field 'format' must be object";
        return response;
    }

    // 3) Mapear JSON -> dominio de forma segura
    auto tournament = std::make_shared<domain::Tournament>();
    domain::from_json(body, *tournament); // usa tu overload por referencia

    // 4) Delegar la creacion
    const std::string id = tournamentDelegate->CreateTournament(tournament);
    if (id.empty()) {
        response.code = crow::CONFLICT;   // insercion fallida/duplicado
        return response;
    }

    response.code = crow::CREATED;
    response.add_header("location", id);
    return response;
}


// GET /tournaments
crow::response TournamentController::ReadAll() const {
    const auto items = tournamentDelegate->ReadAll();

    // gracias a to_json(shared_ptr<Tournament>) en Utilities.hpp,
    // nlohmann puede serializar vector<shared_ptr<Tournament>> directo
    json body = items;

    crow::response res{crow::OK, body.dump()};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return res;
}

// GET /tournaments/<id>
crow::response TournamentController::ReadById(const std::string& id) const {
    // valida formato de id sin depender de Utilities.hpp
    if (!std::regex_match(id, kIdPattern)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if (auto ptr = tournamentDelegate->ReadById(id)) {
        // to_json(shared_ptr<Tournament>) serializa {id,name,format}
        json body = ptr;
        crow::response res{crow::OK, body.dump()};
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return res;
    }

    return crow::response{crow::NOT_FOUND, "tournament not found"};
}

// PUT /tournaments
crow::response TournamentController::UpdateTournament(const crow::request& request) const {
    crow::response response;

    if (!json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }

    json body = json::parse(request.body);
    domain::Tournament t = body.get<domain::Tournament>();

    const bool ok = tournamentDelegate->UpdateTournament(t);
    response.code = ok ? crow::NO_CONTENT : crow::NOT_FOUND;
    return response;
}

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll,          "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, ReadById,         "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments", "PUT"_method)
