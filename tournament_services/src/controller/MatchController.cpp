#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/MatchController.hpp"

#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"
#include "exception/Error.hpp"
#include <iostream>

MatchController::MatchController(const std::shared_ptr<IMatchDelegate>& matchDelegate) : matchDelegate(matchDelegate) {}

static int mapErrorToStatus(const Error err) {
  switch (err) {
    case Error::NOT_FOUND: return crow::NOT_FOUND;
    case Error::INVALID_FORMAT: return crow::BAD_REQUEST;
    case Error::DUPLICATE: return crow::CONFLICT;
    default: return crow::INTERNAL_SERVER_ERROR;
  }
}

// crow::response MatchController::getMatch(const std::string& tournamentId, const std::string& matchId) {
//   auto res = matchDelegate->GetMatch(tournamentId, matchId);
//   if (res) {
//     nlohmann::json body = *res;
//     auto response = crow::response{crow::OK, body.dump()};
//     response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
//     return response;
//   } else {
//     return crow::response{ mapErrorToStatus(res.error()), "Error" };
//   }
// }

crow::response MatchController::getMatches(const std::string& tournamentId) {
  std::cout << "Controller got called7";
  auto res = matchDelegate->GetMatches(tournamentId);
  if (res) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& matchptr : *res) {
      if (matchptr) {
        arr.push_back(*matchptr);
      } else {
        arr.push_back(nullptr);
      }
    }
    auto response = crow::response{crow::OK, arr.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
  } else {
    return crow::response{ mapErrorToStatus(res.error()), "Error" };
  }
}

// crow::response MatchController::updateMatchScore(const crow::request& request, const std::string& tournamentId, const std::string& matchId) {
//   crow::response response;
//   if (!nlohmann::json::accept(request.body)) {
//     response.code = crow::BAD_REQUEST;
//     response.body = "Invalid JSON format";
//     return response;
//   }

//   auto requestBody = nlohmann::json::parse(request.body);
//   domain::Match matchObj = requestBody;

//   if (!matchObj.Id.empty()) {
//     response.code = crow::BAD_REQUEST;
//     response.body = "ID is not editable";
//     return response;
//   }
//   matchObj.Id = matchId;

//   auto res = matchDelegate->UpdateMatchScore(tournamentId, matchObj);
//   if (res) {
//     response.code = crow::OK;
//     response.body = *res;
//     response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
//   } else {
//     response.code = mapErrorToStatus(res.error());
//     response.body = "Error";
//   }

//   return response;
// }

REGISTER_ROUTE(MatchController, getMatches, "/tournaments/<string>/matches", "GET"_method)
// REGISTER_ROUTE(MatchController, getMatch, "/tournaments/<string>/matches/<string>", "GET"_method)
// REGISTER_ROUTE(MatchController, updateMatchScore, "/tournaments/<string>/matches/<string>", "PATCH"_method)
