#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/GroupController.hpp"

#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"
#include <iostream>

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate) : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController()
{
}

crow::response GroupController::GetGroups(const std::string& tournamentId){

    if (!std::regex_match(tournamentId, ID_GROUPVALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format. Must be a valid UUID."};
    }
    
    try {
        auto groups = this->groupDelegate->GetGroups(tournamentId);
        if (groups) {
            const nlohmann::json body = *groups;
            crow::response response{crow::OK, body.dump()};
            response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
            return response;
        } else {
            return crow::response{crow::INTERNAL_SERVER_ERROR, groups.error()};
        }
    } catch (const NotFoundException& e) {
        return crow::response{crow::NOT_FOUND, e.what()};
    } catch (const InvalidFormatException& e) {
        return crow::response{crow::BAD_REQUEST, e.what()};
    }
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId){
    if (!std::regex_match(tournamentId, ID_GROUPVALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format. Must be a valid UUID."};
    }
    if (!std::regex_match(groupId, ID_GROUPVALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid group ID format. Must be a valid UUID."};
    }
    
    try {
        auto group = this->groupDelegate->GetGroup(tournamentId, groupId);
        if (group) {
            const nlohmann::json body = *group;
            crow::response response{crow::OK, body.dump()};
            response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
            return response;
        } else {
            return crow::response{crow::INTERNAL_SERVER_ERROR, group.error()};
        }
    } catch (const NotFoundException& e) {
        return crow::response{crow::NOT_FOUND, e.what()};
    } catch (const InvalidFormatException& e) {
        return crow::response{crow::BAD_REQUEST, e.what()};
    }
}

crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId){
    if (!std::regex_match(tournamentId, ID_GROUPVALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format. Must be a valid UUID."};
    }
    try {
        auto requestBody = nlohmann::json::parse(request.body);
        domain::Group group = requestBody;

        auto groupId = groupDelegate->CreateGroup(tournamentId, group);
        crow::response response;
        if (groupId) {
            response.add_header("location", *groupId);
            response.code = crow::CREATED;
        } else {
            return crow::response{crow::INTERNAL_SERVER_ERROR, groupId.error()};
            // response.code = 422;
            // response.body = groupId.error();
        }
        return response;
    } catch (const NotFoundException& e) {
        return crow::response{crow::NOT_FOUND, e.what()};
    } catch (const InvalidFormatException& e) {
        return crow::response{crow::BAD_REQUEST, e.what()};
    } catch (const DuplicateException& e) {
        return crow::response{crow::CONFLICT, e.what()};
    } catch (const nlohmann::json::parse_error& e) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON in request body"};
    }
}

crow::response GroupController::UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId) {
    if (!std::regex_match(tournamentId, ID_GROUPVALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format. Must be a valid UUID."};
    }
    if (!std::regex_match(groupId, ID_GROUPVALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid group ID format. Must be a valid UUID."};
    }
    try {
        domain::Group group = nlohmann::json::parse(request.body);
        
        auto result = groupDelegate->UpdateGroup(tournamentId, group, groupId);
        
        if (result.has_value()) {
            return crow::response{crow::NO_CONTENT}; // 204 No Content for successful update
        } else {
            return crow::response{crow::INTERNAL_SERVER_ERROR, result.error()};
        }
    } catch (const NotFoundException& e) {
        return crow::response{crow::NOT_FOUND, e.what()};
    } catch (const InvalidFormatException& e) {
        return crow::response{crow::BAD_REQUEST, e.what()};
    } catch (const DuplicateException& e) {
        return crow::response{crow::CONFLICT, e.what()};
    } catch (const nlohmann::json::parse_error& e) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON in request body"};
    }
}

crow::response GroupController::UpdateTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId) {
    const std::vector<domain::Team> teams = nlohmann::json::parse(request.body);
    const auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
    if (result) {
        return crow::response{crow::NO_CONTENT};
    }

    return crow::response{422, result.error()};
}

crow::response GroupController::RemoveGroup(const std::string& tournamentId, const std::string& groupId) {
    if (!std::regex_match(tournamentId, ID_GROUPVALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format. Must be a valid UUID."};
    }
    if (!std::regex_match(groupId, ID_GROUPVALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid group ID format. Must be a valid UUID."};
    }

    try {
        groupDelegate->RemoveGroup(tournamentId, groupId);
        return crow::response{crow::NO_CONTENT};
    } catch (const NotFoundException& e) {
        return crow::response{crow::NOT_FOUND, e.what()};
    } catch (const InvalidFormatException& e) {
        return crow::response{crow::BAD_REQUEST, e.what()};
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, e.what()};
    }
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method) 
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)
REGISTER_ROUTE(GroupController, RemoveGroup, "/tournaments/<string>/groups/<string>", "DELETE"_method)
