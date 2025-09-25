#include <string>
#include <utility>

#include "domain/Team.hpp"
#include "domain/Utilities.hpp"
#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"

static const std::regex ID_VALUE("[A-Za-z0-9\\-]+");

TeamController::TeamController(std::shared_ptr<ITeamDelegate> teamDelegate) : teamDelegate(std::move(teamDelegate)) {}

crow::response TeamController::CreateTeam(const crow::request &request) const {
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
    response.add_header("location", createdId.data());
    return response;
  } catch (const std::exception& ex) {
    std::string errorMsg = ex.what();
    if (errorMsg.find("duplicate key value") != std::string::npos) {
      return crow::response{crow::CONFLICT, "Team with this name already exists."};
    }
    printf("Error creating team: %s\n", ex.what());
    return crow::response{crow::INTERNAL_SERVER_ERROR, "Failed to create team."};
  }
}
crow::response TeamController::getTeam(const std::string &teamId) const {
  if (!std::regex_match(teamId, ID_VALUE)) {
    return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
  }

  if (auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
    nlohmann::json body = team;
    auto response = crow::response{crow::OK, body.dump()};
    response.add_header("Content-Type", "application/json");
    return response;
  }
  return crow::response{crow::NOT_FOUND, "team not found"};
}

crow::response TeamController::getAllTeams() const {
  nlohmann::json body = teamDelegate->GetAllTeams();
  return crow::response{200, body.dump()};
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, CreateTeam, "/teams", "POST"_method)