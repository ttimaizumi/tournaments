//
// Created by tsuny on 8/31/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <string>
#include <utility>
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate) : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    const std::string id = tournamentDelegate->CreateTournament(tournament);
    crow::response response;
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


REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)