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

    const std::string id = tournamentDelegate->CreateTournament(tournament);

    if(id.empty()) {
        response.code = crow::CONFLICT; // 409
        return response;
    }

    response.code = crow::CREATED;
    response.add_header("location", id);
    return response;
}

crow::response TournamentController::ReadAll() const {
    nlohmann::json body = tournamentDelegate->ReadAll();
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

    if(auto tournament = tournamentDelegate->ReadById(tournamentId); tournament != nullptr) {
        nlohmann::json body = tournament;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::NOT_FOUND, "tournament not found"};
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

    std::string updatedId(tournamentDelegate->UpdateTournament(tournament));

    crow::response response;
    response.code = crow::OK;
    response.add_header("location", updatedId);
    return response;
}

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, GetTournament, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>", "PATCH"_method)