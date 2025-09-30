//
// Created by tsuny on 8/31/25.
//

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <string>
#include <utility>
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"
#include "configuration/RouteDefinition.hpp"
// #include "exception/Duplicate.hpp"
// #include "exception/NotFound.hpp"
#include "configuration/RouteDefinition.hpp"
#include <iostream>

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate) : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::getTournament(const std::string& tournamentId) const
{
    if(!std::regex_match(tournamentId, ID_VALUE_TOURNAMENT))
    {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    auto tournament = tournamentDelegate -> GetTournament(tournamentId);
    if(tournament == nullptr)
    {
        return crow::response{crow::NOT_FOUND, "Tournament not found"};
    }

    nlohmann::json body = tournament;
    auto response = crow::response{crow::OK, body.dump()};
    response.add_header("Content-Type", "application/json");
    return response;
}

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

    return response;
}


REGISTER_ROUTE(TournamentController, getTournament, "/tournaments/<string>", "GET"_method)

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)