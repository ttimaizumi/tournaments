//
// Created by tsuny on 8/31/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"
#include "exception/Error.hpp"
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <iostream>

TournamentController::TournamentController(const std::shared_ptr<ITournamentDelegate>& delegate)
    : tournamentDelegate(delegate) {}

TournamentController::~TournamentController() {}

static int mapErrorToStatus(const Error err) {
    switch (err) {
        case Error::NOT_FOUND: return crow::NOT_FOUND;
        case Error::INVALID_FORMAT: return crow::BAD_REQUEST;
        case Error::DUPLICATE: return crow::CONFLICT;
        default: return crow::INTERNAL_SERVER_ERROR;
    }
}

crow::response TournamentController::getTournament(const std::string& tournamentId) {
    if (!std::regex_match(tournamentId, ID_VALUE_TOURNAMENT)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    auto res = tournamentDelegate->GetTournament(tournamentId);
    if (res) {
        nlohmann::json body = *res;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    } else {
        return crow::response{mapErrorToStatus(res.error()), "Error"};
    }
}

crow::response TournamentController::ReadAll() {
    auto res = tournamentDelegate->ReadAll();
    if (res) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& tournamentptr : *res) {
            if (tournamentptr) {
                arr.push_back(*tournamentptr);
            } else {
                arr.push_back(nullptr);
            }
        }
        auto response = crow::response{crow::OK, arr.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    } else {
        return crow::response{mapErrorToStatus(res.error()), "Error"};
    }
}

crow::response TournamentController::CreateTournament(const crow::request& request) {
    crow::response response;

    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Tournament tournament = requestBody;

    auto res = tournamentDelegate->CreateTournament(tournament);
    if (res) {
        response.code = crow::CREATED;
        response.add_header("Location", *res);
        response.body = "";
    } else {
        response.code = mapErrorToStatus(res.error());
        response.body = "Error";
    }

    return response;
}

crow::response TournamentController::updateTournament(const crow::request& request, const std::string& tournamentId) {
    crow::response response;
    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid JSON format";
        return response;
    }

    auto requestBody = nlohmann::json::parse(request.body);
    domain::Tournament tournamentObj = requestBody;

    if (!tournamentObj.Id().empty()) {
        response.code = crow::BAD_REQUEST;
        response.body = "ID is not editable";
        return response;
    }
    tournamentObj.Id() = tournamentId;

    auto res = tournamentDelegate->UpdateTournament(tournamentObj);
    if (res) {
        response.code = crow::NO_CONTENT;
        response.body = "";
    } else {
        response.code = mapErrorToStatus(res.error());
        response.body = "Error";
    }

    return response;
}

crow::response TournamentController::deleteTournament(const std::string& tournamentId) {
    crow::response response;

    if (!std::regex_match(tournamentId, ID_VALUE_TOURNAMENT)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid ID format";
        return response;
    }

    auto res = tournamentDelegate->DeleteTournament(tournamentId);
    if (res) {
        response.code = crow::NO_CONTENT;
        response.body = "";
    } else {
        response.code = mapErrorToStatus(res.error());
        response.body = "Error";
    }

    return response;
}

REGISTER_ROUTE(TournamentController, getTournament, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, updateTournament, "/tournaments/<string>", "PATCH"_method)
REGISTER_ROUTE(TournamentController, deleteTournament, "/tournaments/<string>", "DELETE"_method)
REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)