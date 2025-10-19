#include "controller/GroupController.hpp"
#include <nlohmann/json.hpp>
#include "configuration/RouteDefinition.hpp"

using nlohmann::json;

// --- Helpers (solo en este TU)
static json groupToJson(const domain::Group& g) {
    json j;
    j["id"]           = g.Id();
    j["name"]         = g.Name();
    j["tournamentId"] = g.TournamentId();
    json arr = json::array();
    for (const auto& t : g.Teams()) {
        arr.push_back(json{ {"id", t.Id}, {"name", t.Name} });
    }
    j["teams"] = std::move(arr);
    return j;
}

static domain::Group jsonToGroup(const json& j) {
    domain::Group g;
    if (j.contains("id"))           g.Id()           = j.at("id").get<std::string>();
    if (j.contains("name"))         g.Name()         = j.at("name").get<std::string>();
    if (j.contains("tournamentId")) g.TournamentId() = j.at("tournamentId").get<std::string>();
    if (j.contains("teams") && j.at("teams").is_array()) {
        auto& vec = g.Teams();
        for (const auto& it : j.at("teams")) {
            domain::Team t;
            if (it.contains("id"))   t.Id   = it.at("id").get<std::string>();
            if (it.contains("name")) t.Name = it.at("name").get<std::string>();
            vec.push_back(std::move(t));
        }
    }
    return g;
}

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate)
    : groupDelegate(delegate) {}

GroupController::~GroupController() = default;

crow::response GroupController::GetGroups(const std::string& tournamentId) {
    try {
        auto r = groupDelegate->GetGroups(tournamentId);
        if (!r.has_value()) return crow::response{500, r.error()};

        json arr = json::array();
        for (auto& gp : r.value()) {
            if (gp) arr.push_back(groupToJson(*gp));
        }

        crow::response res{crow::OK, arr.dump()};
        res.add_header("content-type", "application/json");
        return res;
    } catch (const std::exception& e) {
        return crow::response{500, std::string("exception: ") + e.what()};
    }
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId) {
    try {
        auto r = groupDelegate->GetGroup(tournamentId, groupId);
        if (!r.has_value()) return crow::response{500, r.error()};
        if (!r.value())     return crow::response{crow::NOT_FOUND};

        crow::response res{crow::OK, groupToJson(*r.value()).dump()};
        res.add_header("content-type", "application/json");
        return res;
    } catch (const std::exception& e) {
        return crow::response{500, std::string("exception: ") + e.what()};
    }
}

crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId) {
    try {
        const json j = json::parse(request.body, nullptr, /*allow_exceptions*/ true);
        if (!j.contains("id") || !j.contains("name"))
            return crow::response{crow::BAD_REQUEST, "missing id or name"};

        auto g = jsonToGroup(j);
        auto r = groupDelegate->CreateGroup(tournamentId, g);
        if (!r.has_value()) {
            const auto& err = r.error();
            if (err == "tournament-not-found")       return crow::response{422, "Tournament does not exist"};
            if (err == "group-already-exists")       return crow::response{409, "Group already exists"};
            if (err.rfind("team-not-found:", 0) == 0) return crow::response{422, err};
            if (err == "misconfigured-dependencies") return crow::response{500, "GroupDelegate not wired"};
            return crow::response{500, err};
        }

        crow::response res{crow::CREATED};
        res.add_header("location", r.value());
        return res;
    } catch (const std::exception& e) {
        return crow::response{crow::BAD_REQUEST, std::string("bad json: ") + e.what()};
    }
}

crow::response GroupController::UpdateGroup(const crow::request& request,
                                            const std::string& tournamentId,
                                            const std::string& groupId) {
    try {
        const json j = json::parse(request.body, nullptr, true);
        auto incoming = jsonToGroup(j);
        incoming.Id()           = groupId;
        incoming.TournamentId() = tournamentId;

        auto r = groupDelegate->UpdateGroup(tournamentId, incoming);
        if (!r.has_value()) {
            const auto& err = r.error();
            if (err == "group-not-found")            return crow::response{crow::NOT_FOUND};
            if (err == "misconfigured-dependencies") return crow::response{500, "GroupDelegate not wired"};
            return crow::response{500, err};
        }
        return crow::response{crow::NO_CONTENT};
    } catch (const std::exception& e) {
        return crow::response{crow::BAD_REQUEST, std::string("bad json: ") + e.what()};
    }
}

crow::response GroupController::UpdateTeams(const crow::request& request,
                                            const std::string& tournamentId,
                                            const std::string& groupId) {
    try {
        const json j = json::parse(request.body, nullptr, true);
        if (!j.is_array()) return crow::response{crow::BAD_REQUEST, "expect array"};

        std::vector<domain::Team> teams;
        for (const auto& it : j) {
            domain::Team t;
            if (it.contains("id"))   t.Id   = it.at("id").get<std::string>();
            if (it.contains("name")) t.Name = it.at("name").get<std::string>();
            teams.push_back(std::move(t));
        }

        auto r = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
        if (!r.has_value()) {
            const auto& err = r.error();
            if (err == "group-not-found")                 return crow::response{422, "Group does not exist"};
            if (err == "group-full")                      return crow::response{422, "Group at max capacity"};
            if (err.rfind("team-not-found:", 0) == 0)     return crow::response{422, err};
            if (err.rfind("team-already-in-group:", 0)==0) return crow::response{422, err};
            if (err == "misconfigured-dependencies")      return crow::response{500, "GroupDelegate not wired"};
            return crow::response{500, err};
        }
        return crow::response{crow::NO_CONTENT};
    } catch (const std::exception& e) {
        return crow::response{crow::BAD_REQUEST, std::string("bad json: ") + e.what()};
    }
}

// --- Registrar rutas UNA sola vez en este .cpp
REGISTER_ROUTE(GroupController, GetGroups,   "/tournaments/<string>/groups",                 "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup,    "/tournaments/<string>/groups/<string>",       "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups",                 "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>",       "PATCH"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)
