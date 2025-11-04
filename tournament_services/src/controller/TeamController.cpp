#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"

#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"
#include "exception/Error.hpp"
#include <iostream>

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate) : teamDelegate(teamDelegate) {}

static int mapErrorToStatus(const Error err) {
  switch (err) {
    case Error::NOT_FOUND: return crow::NOT_FOUND;
    case Error::INVALID_FORMAT: return crow::BAD_REQUEST;
    case Error::DUPLICATE: return crow::CONFLICT;
    default: return crow::INTERNAL_SERVER_ERROR;
  }
}

crow::response TeamController::getTeam(const std::string& teamId) const {
  if (!std::regex_match(teamId, ID_VALUE)) {
    return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
  }

  auto res = teamDelegate->GetTeam(teamId);
  if (res) {
    nlohmann::json body = *res;
    auto response = crow::response{crow::OK, body.dump()};
    response.add_header("Content-Type", "application/json");
    return response;
  } else {
    return crow::response{ mapErrorToStatus(res.error()), "Error" };
  }
}

crow::response TeamController::getAllTeams() const {
  auto res = teamDelegate->GetAllTeams();
  if (res) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& teamptr : *res) {
      if (teamptr) {
        arr.push_back(*teamptr);
      } else {
        arr.push_back(nullptr);
      }
    }
    auto response = crow::response{crow::OK, arr.dump()};
    response.add_header("Content-Type", "application/json");
    return response;
  } else {
    return crow::response{ mapErrorToStatus(res.error()), "Error" };
  }
}

crow::response TeamController::createTeam(const crow::request& request) const {
  crow::response response;

  if (!nlohmann::json::accept(request.body)) {
    response.code = crow::BAD_REQUEST;
    return response;
  }
  auto requestBody = nlohmann::json::parse(request.body);
  domain::Team team = requestBody;

  auto res = teamDelegate->CreateTeam(team);
  if (res) {
    response.code = crow::CREATED;
    response.body = *res;
    response.add_header("Content-Type", "text/plain");
  } else {
    response.code = mapErrorToStatus(res.error());
    response.body = "Error";
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

  auto requestBody = nlohmann::json::parse(request.body);
  domain::Team teamObj = requestBody;

  if (!teamObj.Id.empty()) {
    response.code = crow::BAD_REQUEST;
    response.body = "ID is not editable";
    return response;
  }
  teamObj.Id = teamId;

  auto res = teamDelegate->UpdateTeam(teamObj);
  if (res) {
    response.code = crow::OK;
    response.body = *res;
    response.add_header("Content-Type", "application/json");
  } else {
    response.code = mapErrorToStatus(res.error());
    response.body = "Error";
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

  auto res = teamDelegate->DeleteTeam(teamId);
  if (res) {
    response.code = crow::NO_CONTENT;
    response.body = "";
  } else {
    response.code = mapErrorToStatus(res.error());
    response.body = "Error";
  }

  return response;
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, createTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, updateTeam, "/teams/<string>", "PATCH"_method)
REGISTER_ROUTE(TeamController, deleteTeam, "/teams/<string>", "DELETE"_method)
