#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/GroupController.hpp"
#include "domain/Utilities.hpp"

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate)
    : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController() = default;

crow::response GroupController::GetGroups(const std::string& tournamentId) {
    auto groups = groupDelegate->GetGroups(tournamentId);
    if (!groups.has_value())
        return crow::response{crow::NOT_FOUND, groups.error()};

    nlohmann::json body = *groups;
    crow::response res{crow::OK, body.dump()};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return res;
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId) {
    auto result = groupDelegate->GetGroup(tournamentId, groupId);
    if (!result.has_value())
        return crow::response{crow::NOT_FOUND, result.error()};

    auto group = result.value();
    if (group == nullptr)
        return crow::response{crow::NOT_FOUND, "Group not found"};

    nlohmann::json body = group;
    crow::response res{crow::OK, body.dump()};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return res;
}

crow::response GroupController::CreateGroup(const crow::request& req, const std::string& tournamentId) {
    try {
        auto body = nlohmann::json::parse(req.body);
        domain::Group group = body;

        auto id = groupDelegate->CreateGroup(tournamentId, group);
        if (id.has_value()) {
            crow::response res{crow::CREATED};
            res.add_header("location", id.value());
            res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
            return res;
        }

        const std::string& err = id.error();
        if (err.find("already exists") != std::string::npos)
            return crow::response{409, err};
        if (err.find("Tournament doesn't exist") != std::string::npos)
            return crow::response{crow::NOT_FOUND, err};
        return crow::response{422, err};
    } catch (...) {
        return crow::response{422};
    }
}

crow::response GroupController::UpdateGroup(
    const crow::request& req,
    const std::string& tournamentId,
    const std::string& groupId) {
    try {
        auto body = nlohmann::json::parse(req.body);
        domain::Group group = body;
        group.SetId(groupId);

        auto result = groupDelegate->UpdateGroup(tournamentId, group);
        if (result.has_value())
            return crow::response{crow::NO_CONTENT};

        const std::string& err = result.error();
        if (err.find("doesn't exist") != std::string::npos)
            return crow::response{crow::NOT_FOUND, err};
        return crow::response{422, err};
    } catch (...) {
        return crow::response{422};
    }
}

crow::response GroupController::RemoveGroup(const std::string& tournamentId, const std::string& groupId) {
    auto result = groupDelegate->RemoveGroup(tournamentId, groupId);
    if (result.has_value())
        return crow::response{crow::NO_CONTENT};

    const std::string& err = result.error();
    if (err.find("doesn't exist") != std::string::npos)
        return crow::response{crow::NOT_FOUND, err};
    return crow::response{422, err};
}

crow::response GroupController::UpdateTeams(const crow::request& req, const std::string& tournamentId, const std::string& groupId) {
    try {
        auto body = nlohmann::json::parse(req.body);
        std::vector<domain::Team> teams;
        if (body.is_array())
            for (auto& t : body) teams.emplace_back(t.get<domain::Team>());
        else
            teams.emplace_back(body.get<domain::Team>());

        auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
        if (result.has_value())
            return crow::response{crow::NO_CONTENT};

        const std::string& err = result.error();
        if (err.find("doesn't exist") != std::string::npos)
            return crow::response{crow::NOT_FOUND, err};
        return crow::response{422, err};
    } catch (...) {
        return crow::response{422};
    }
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)
REGISTER_ROUTE(GroupController, RemoveGroup, "/tournaments/<string>/groups/<string>", "DELETE"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "POST"_method)
