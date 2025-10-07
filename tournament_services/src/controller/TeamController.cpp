//
// Created by root on 9/27/25.
//

#include "controller/TeamController.hpp"

#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"
#include <iostream>

TeamController::TeamController(
    const std::shared_ptr<ITeamDelegate>& teamDelegate)
    : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
  if (!std::regex_match(teamId, ID_VALUE)) {
    return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
  }
  try {
    auto team = teamDelegate->GetTeam(teamId);
    nlohmann::json body = team;
    auto response = crow::response{crow::OK, body.dump()};
    response.add_header("Content-Type", "application/json");
    return response;
  }
  catch (const NotFoundException& e) {
    return crow::response{crow::NOT_FOUND, e.what()};

  } catch (const InvalidFormatException& e) {
    return crow::response{crow::BAD_REQUEST, e.what()};

  } catch (const std::exception& e) {
    return crow::response{crow::INTERNAL_SERVER_ERROR, "Internal server error"};
  }
}

crow::response TeamController::getAllTeams() const {
  nlohmann::json body = teamDelegate->GetAllTeams();
  return crow::response{200, body.dump()};
}

crow::response TeamController::createTeam(const crow::request& request) const {
  crow::response response;

  if (!nlohmann::json::accept(request.body)) {
    response.code = crow::BAD_REQUEST;
    return response;
  }
  auto requestBody = nlohmann::json::parse(request.body);
  domain::Team team = requestBody;

  try {
    auto createdId = teamDelegate->CreateTeam(team);
    response.code = crow::CREATED;
    response.body = createdId;
  } catch (const std::invalid_argument& e) {
    response.code = crow::BAD_REQUEST;
    response.body = e.what();

  } catch (const DuplicateException& e) {
    response.code = crow::CONFLICT;
    response.body = e.what();
  } catch (const std::exception& e) {
    response.code = crow::INTERNAL_SERVER_ERROR;
    response.body = "Internal server error";
  }

  return response;
}

crow::response TeamController::updateTeam(const crow::request& request, const std::string& teamId) const {
  crow::response response;
  if (!nlohmann::json::accept(request.body)) {
    response.code = crow::BAD_REQUEST;
    response.body = "Invalid JSON format";
    return response;
  }

  // Parse the body into a Team object
  auto requestBody = nlohmann::json::parse(request.body);
  domain::Team teamObj = requestBody;

  // Validate the Team object (might not be necessary if nlohmann::json does this)
  // try {
  //   teamObj = ;
  // } catch (const nlohmann::json::exception& e) {
  //   response.code = crow::BAD_REQUEST;
  //   response.body = "Request body does not match Team structure";
  //   return response;
  // }

  // Stop users from changing the ID via the body
  if (!teamObj.Id.empty()) {
    response.code = crow::BAD_REQUEST;
    response.body = "ID is not editable";
    return response;
  }
  // Set the ID to the path parameter to ensure they match
  teamObj.Id = teamId;

  // Try to PATCH the team
  try {
    auto team = teamDelegate->UpdateTeam(teamObj);
    response.code = crow::OK;
    response.body = team;
    
  } catch (const NotFoundException& e) {
    response.code = crow::NOT_FOUND;
    response.body = e.what();

  } catch (const InvalidFormatException& e) {
    response.code = crow::BAD_REQUEST;
    response.body = e.what();
    
  } catch (const std::exception& e) {
    response.code = crow::INTERNAL_SERVER_ERROR;
    response.body = "Internal server error";
  }

  return response;
}

crow::response TeamController::deleteTeam(const std::string& teamId) const {
  crow::response response;

  if (!std::regex_match(teamId, ID_VALUE)) {
    response.code = crow::BAD_REQUEST;
    response.body = "Invalid ID format";
    return response;
  }

  try {
    teamDelegate->DeleteTeam(teamId);
    response.code = crow::NO_CONTENT;
    response.body = "";
    
  } catch (const NotFoundException& e) {
    response.code = crow::NOT_FOUND;
    response.body = e.what();
  } catch (const InvalidFormatException& e) {
    response.code = crow::BAD_REQUEST;
    response.body = e.what();

  } catch (const std::exception& e) {
    response.code = crow::INTERNAL_SERVER_ERROR;
    response.body = "Internal server error";
  }

  return response;
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, createTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, updateTeam, "/teams/<string>", "PATCH"_method)
REGISTER_ROUTE(TeamController, deleteTeam, "/teams/<string>", "DELETE"_method)
