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
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
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
    crow::response response;
    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Tournament tournament;

    try {
        auto createdId = tournamentDelegate->CreateTournament(tournament);
        response.code = crow::CREATED;
        response.body = createdId;
    } catch (const DuplicateException& e) {
        response.code = crow::CONFLICT;
        response.body = e.what();
    }catch (const std::exception& e) {
        response.code = crow::INTERNAL_SERVER_ERROR;
        response.body = "Internal server error";
    }

    return response;

}

crow::response TournamentController::ReadAll() const {
    nlohmann::json body = tournamentDelegate->ReadAll();
    crow::response response;
    response.code = crow::OK;
    response.body = body.dump();

    return response;
}

crow::response TournamentController::updateTournament(const crow::request& request, const std::string& tournamentId) const {
    crow::response response;

    if (!std::regex_match(tournamentId, ID_VALUE_TOURNAMENT)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid ID format";
        return response;
    }

    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid JSON format";
        return response;
    }

    auto requestBody = nlohmann::json::parse(request.body);
    domain::Tournament tournamentObj;
    try {
        tournamentObj = requestBody;
    } catch (const nlohmann::json::exception& e) {
        response.code = crow::BAD_REQUEST;
        response.body = "Request body does not match Tournament structure";
        return response;
    }

    if (!tournamentObj.Id().empty() && tournamentObj.Id() != tournamentId) {
        response.code = crow::BAD_REQUEST;
        response.body = "ID is not editable";
        return response;
    }

    tournamentObj.Id() = tournamentId;

    try {
        tournamentDelegate->UpdateTournament(tournamentObj);
        response.code = crow::NO_CONTENT;  // 204
    }
    catch (const NotFoundException& e) {
        response.code = crow::NOT_FOUND; 
        response.body = e.what();
    }
    catch (const std::exception& e) {
        response.code = crow::INTERNAL_SERVER_ERROR;
        response.body = "Internal server error";
    }

    return response;
}

REGISTER_ROUTE(TournamentController, getTournament, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, updateTournament, "/tournaments/<string>", "PATCH"_method)
//delete y modificar update
REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)