//
// Created by root on 10/8/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/GroupController.hpp"
#include "domain/Utilities.hpp"

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate) : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController()
{
}

crow::response GroupController::GetGroups(const std::string& tournamentId) {
    if (auto groups = groupDelegate->GetGroups(tournamentId)) {
        nlohmann::json body = *groups;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{422};
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId) {
    auto groupResult = groupDelegate->GetGroup(tournamentId, groupId);

    if (!groupResult.has_value()) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, groupResult.error()};
    }

    auto group = groupResult.value();
    if (group == nullptr) {
        return crow::response{crow::NOT_FOUND, "Group not found"};
    }

    nlohmann::json body = group;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId) {
    try {
        auto requestBody = nlohmann::json::parse(request.body);
        domain::Group group = requestBody;

        auto groupId = groupDelegate->CreateGroup(tournamentId, group);
        crow::response response;
        response.code = groupId ? crow::CREATED : 422;
        if (groupId) response.add_header("location", *groupId);
        return response;
    } catch (const nlohmann::json::exception&) {
        return crow::response{422};
    }
}

crow::response GroupController::UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId) {
    try {
        auto requestBody = nlohmann::json::parse(request.body);
        domain::Group group = requestBody;
        group.Id() = groupId;

        auto result = groupDelegate->UpdateGroup(tournamentId, group);

        if (!result.has_value()) {
            if (result.error().find("doesn't exist") != std::string::npos) {
                return crow::response{crow::NOT_FOUND, result.error()};
            }
            return crow::response{422, result.error()};
        }

        return crow::response{crow::NO_CONTENT};
    } catch (const nlohmann::json::exception&) {
        return crow::response{422};
    }
}

crow::response GroupController::RemoveGroup(const std::string& tournamentId, const std::string& groupId) {
    auto result = groupDelegate->RemoveGroup(tournamentId, groupId);

    if (!result.has_value()) {
        if (result.error().find("doesn't exist") != std::string::npos) {
            return crow::response{crow::NOT_FOUND, result.error()};
        }
        return crow::response{422, result.error()};
    }

    return crow::response{crow::NO_CONTENT};
}

crow::response GroupController::UpdateTeams(
        const crow::request& request,
        const std::string& tournamentId,
        const std::string& groupId
) {
    try {
        domain::Team team = nlohmann::json::parse(request.body).get<domain::Team>();
        std::vector<domain::Team> teams = { team };

        const auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
        crow::response response;

        if (result.has_value()) {
            response.code = crow::status::CREATED;
        } else {
            response.code = 422;
            response.body = result.error();
        }

        return response;

    } catch (const nlohmann::json::exception& e) {
        crow::response res{422};
        res.body = std::string("JSON parse error: ") + e.what();
        return res;
    }
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)
REGISTER_ROUTE(GroupController, RemoveGroup, "/tournaments/<string>/groups/<string>", "DELETE"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "POST"_method)
