//
// Created by tsuny on 8/31/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <string>
#include <utility>
#include <regex>
#include  "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate) : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
    crow::response response;

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }

    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    auto result = tournamentDelegate->CreateTournament(tournament);

    if(!result.has_value()) {
        const std::string& error = result.error();
        if(error.find("already exists") != std::string::npos) {
            response.code = crow::CONFLICT;
        } else {
            response.code = crow::INTERNAL_SERVER_ERROR;
            response.body = error;
        }
        return response;
    }

    response.code = crow::CREATED;
    response.add_header("location", result.value());
    return response;
}

crow::response TournamentController::ReadAll() const {
    auto result = tournamentDelegate->ReadAll();

    if(!result.has_value()) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    nlohmann::json body = result.value();
    crow::response response;
    response.code = crow::OK;
    response.body = body.dump();
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response TournamentController::GetTournament(const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    auto result = tournamentDelegate->ReadById(tournamentId);

    if(!result.has_value()) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    auto tournament = result.value();
    if(tournament == nullptr) {
        return crow::response{crow::NOT_FOUND, "tournament not found"};
    }

    nlohmann::json body = tournament;
    auto response = crow::response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response TournamentController::UpdateTournament(const crow::request& request, const std::string& tournamentId) const {
    if (!std::regex_match(tournamentId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
    }

    nlohmann::json body = nlohmann::json::parse(request.body);
    domain::Tournament tournament = body;
    tournament.Id() = tournamentId;

    auto result = tournamentDelegate->UpdateTournament(tournament);

    if(!result.has_value()) {
        const std::string& error = result.error();
        if(error.find("not found") != std::string::npos) {
            return crow::response{crow::NOT_FOUND};
        } else {
            return crow::response{crow::INTERNAL_SERVER_ERROR, error};
        }
    }

    return crow::response{crow::NO_CONTENT};
}

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, GetTournament, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>", "PATCH"_method)
