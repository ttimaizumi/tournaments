#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "domain/Group.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "controller/GroupController.hpp"
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"

class GroupDelegateMock : public IGroupDelegate {
    public:
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>), GetGroup, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), CreateGroup, (const std::string_view& tournamentId, const domain::Group& group), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>), GetGroups, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateGroup, (const std::string_view& tournamentId, const domain::Group& group, const std::string_view& groupId), (override)); 
    MOCK_METHOD((std::expected<void, std::string>), RemoveGroup, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateTeams, (const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams), (override));
};

class GroupControllerTest : public ::testing::Test {
    protected:
    std::shared_ptr<GroupDelegateMock> groupDelegateMock;
    std::shared_ptr<GroupController> groupController;

    void SetUp() override {
        groupDelegateMock = std::make_shared<GroupDelegateMock>();
        groupController = std::make_shared<GroupController>(groupDelegateMock);
    }
};

// GET /tournaments/{tournamentId}/groups
// Good path No teams - 200
TEST_F(GroupControllerTest, GetGroups) {
    std::shared_ptr<domain::Group> expectedGroup = std::make_shared<domain::Group>(domain::Group{"Group Name", "12345678-1234-1234-1234-123456789012"});
    expectedGroup->TournamentId() = "12345678-1234-1234-1234-123456789abc";

    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq(std::string("12345678-1234-1234-1234-123456789abc"))))
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>{std::vector<std::shared_ptr<domain::Group>>{expectedGroup}}));

    crow::response response = groupController->GetGroups("12345678-1234-1234-1234-123456789abc");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    ASSERT_EQ(jsonResponse.size(), 1);
}

// Good path With teams - 200
TEST_F(GroupControllerTest, GetGroupsWithTeams) {
    domain::Team team1{"87654321-4321-4321-4321-123456789012", "Team One"};
    domain::Team team2{"87654321-4321-4321-4321-123456789013", "Team Two"};
    
    std::shared_ptr<domain::Group> expectedGroup = std::make_shared<domain::Group>(domain::Group{"Group Name", "12345678-1234-1234-1234-123456789012"});
    expectedGroup->TournamentId() = "12345678-1234-1234-1234-123456789abc";
    expectedGroup->Teams().push_back(team1);
    expectedGroup->Teams().push_back(team2);

    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq(std::string("12345678-1234-1234-1234-123456789abc"))))
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>{std::vector<std::shared_ptr<domain::Group>>{expectedGroup}}));

    crow::response response = groupController->GetGroups("12345678-1234-1234-1234-123456789abc");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    ASSERT_EQ(jsonResponse.size(), 1);
    EXPECT_EQ(jsonResponse[0]["id"], expectedGroup->Id());
    EXPECT_EQ(jsonResponse[0]["name"], expectedGroup->Name());
    ASSERT_EQ(jsonResponse[0]["teams"].size(), 2);
    EXPECT_EQ(jsonResponse[0]["teams"][0]["id"], team1.Id);
    EXPECT_EQ(jsonResponse[0]["teams"][0]["name"], team1.Name);
    EXPECT_EQ(jsonResponse[0]["teams"][1]["id"], team2.Id);
    EXPECT_EQ(jsonResponse[0]["teams"][1]["name"], team2.Name);
}

// TournamentId not found - 404
TEST_F(GroupControllerTest, GetGroupsTournamentNotFound) {
    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq(std::string("12345678-1234-1234-1234-123456789def"))))
        .WillOnce(testing::Throw(NotFoundException("Tournament not found")));

    crow::response response = groupController->GetGroups("12345678-1234-1234-1234-123456789def");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("Tournament not found", response.body);
}

// Invalid format tournamentId - 400
TEST_F(GroupControllerTest, GetGroupsInvalidTournamentIdFormat) {
    crow::response response = groupController->GetGroups("");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid tournament ID format. Must be a valid UUID.", response.body);

    response = groupController->GetGroups("invalid@tournament#id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid tournament ID format. Must be a valid UUID.", response.body);
}



// POST /tournaments/{tournamentId}/groups
// Test group creation with valid JSON - validate JSON to domain transformation and HTTP 201 response
TEST_F(GroupControllerTest, CreateGroup_ValidJsonTransformation_ReturnsHttp201) {
    std::string tournamentId = "12345678-1234-1234-1234-123456789abc";
    std::string expectedGroupId = "87654321-4321-4321-4321-123456789012";
    
    nlohmann::json requestJson = {
        {"name", "Test Group"},
        {"teams", nlohmann::json::array({
            {{"id", "team-id-1"}, {"name", "Team One"}},
            {{"id", "team-id-2"}, {"name", "Team Two"}}
        })}
    };
    
    domain::Group expectedGroup{"Test Group"};
    expectedGroup.TournamentId() = tournamentId;
    expectedGroup.Teams().push_back(domain::Team{"team-id-1", "Team One"});
    expectedGroup.Teams().push_back(domain::Team{"team-id-2", "Team Two"});
    
    EXPECT_CALL(*groupDelegateMock, CreateGroup(
        testing::Eq(tournamentId),
        testing::AllOf(
            testing::Property(&domain::Group::Name, testing::Eq("Test Group")),
            testing::Property(&domain::Group::Teams, testing::SizeIs(2))
        )
    )).WillOnce(testing::Return(std::expected<std::string, std::string>{expectedGroupId}));
    
    crow::request request;
    request.body = requestJson.dump();
    
    crow::response response = groupController->CreateGroup(request, tournamentId);
    
    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(expectedGroupId, response.get_header_value("location"));
}

// POST /tournaments/{tournamentId}/groups
// Test group creation with database error - validate HTTP 409 response
TEST_F(GroupControllerTest, CreateGroup_DatabaseDuplicateError_ReturnsHttp409) {
    std::string tournamentId = "12345678-1234-1234-1234-123456789abc";
    
    nlohmann::json requestJson = {
        {"name", "Duplicate Group"}
    };
    
    EXPECT_CALL(*groupDelegateMock, CreateGroup(
        testing::Eq(tournamentId),
        testing::Property(&domain::Group::Name, testing::Eq("Duplicate Group"))
    )).WillOnce(testing::Throw(DuplicateException("A group with the same name already exists in this tournament.")));
    
    crow::request request;
    request.body = requestJson.dump();
    
    crow::response response = groupController->CreateGroup(request, tournamentId);
    
    EXPECT_EQ(crow::CONFLICT, response.code);
    EXPECT_EQ("A group with the same name already exists in this tournament.", response.body);
}

// GET /tournaments/{tournamentId}/groups/{groupId}
// Test group search by ID - validate parameters passed to delegate and HTTP 200 response
TEST_F(GroupControllerTest, GetGroup_ValidIds_ReturnsHttp200) {
    std::string tournamentId = "12345678-1234-1234-1234-123456789abc";
    std::string groupId = "87654321-4321-4321-4321-123456789012";
    
    std::shared_ptr<domain::Group> expectedGroup = std::make_shared<domain::Group>("Test Group", groupId);
    expectedGroup->TournamentId() = tournamentId;
    expectedGroup->Teams().push_back(domain::Team{"team-id-1", "Team One"});
    expectedGroup->Teams().push_back(domain::Team{"team-id-2", "Team Two"});
    
    EXPECT_CALL(*groupDelegateMock, GetGroup(
        testing::Eq(tournamentId),
        testing::Eq(groupId)
    )).WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>{expectedGroup}));
    
    crow::response response = groupController->GetGroup(tournamentId, groupId);
    
    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
    
    auto jsonResponse = crow::json::load(response.body);
    EXPECT_EQ(expectedGroup->Id(), jsonResponse["id"]);
    EXPECT_EQ(expectedGroup->Name(), jsonResponse["name"]);
    EXPECT_EQ(expectedGroup->TournamentId(), jsonResponse["tournamentId"]);
    ASSERT_EQ(jsonResponse["teams"].size(), 2);
    EXPECT_EQ("team-id-1", jsonResponse["teams"][0]["id"]);
    EXPECT_EQ("Team One", jsonResponse["teams"][0]["name"]);
}

// GET /tournaments/{tournamentId}/groups/{groupId}
// Test group search by ID - simulate not found and validate HTTP 404 response
TEST_F(GroupControllerTest, GetGroup_GroupNotFound_ReturnsHttp404) {
    std::string tournamentId = "12345678-1234-1234-1234-123456789abc";
    std::string groupId = "87654321-4321-4321-4321-123456789999";
    
    EXPECT_CALL(*groupDelegateMock, GetGroup(
        testing::Eq(tournamentId),
        testing::Eq(groupId)
    )).WillOnce(testing::Throw(NotFoundException("Group not found")));
    
    crow::response response = groupController->GetGroup(tournamentId, groupId);
    
    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("Group not found", response.body);
}

// PATCH /tournaments/{tournamentId}/groups/{groupId}
// Test group update with valid JSON - validate JSON to domain transformation and HTTP 204 response
TEST_F(GroupControllerTest, UpdateGroup_ValidJsonTransformation_ReturnsHttp204) {
    std::string tournamentId = "12345678-1234-1234-1234-123456789abc";
    std::string groupId = "87654321-4321-4321-4321-123456789012";
    
    nlohmann::json requestJson = {
        {"name", "Updated Group Name"},
        {"teams", nlohmann::json::array({
            {{"id", "team-id-3"}, {"name", "Team Three"}}
        })}
    };
    
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(
        testing::Eq(tournamentId),
        testing::AllOf(
            testing::Property(&domain::Group::Name, testing::Eq("Updated Group Name")),
            testing::Property(&domain::Group::Teams, testing::SizeIs(1))
        ),
        testing::Eq(groupId)
    )).WillOnce(testing::Return(std::expected<void, std::string>{}));
    
    crow::request request;
    request.body = requestJson.dump();
    
    crow::response response = groupController->UpdateGroup(request, tournamentId, groupId);
    
    EXPECT_EQ(crow::NO_CONTENT, response.code);
}

// PATCH /tournaments/{tournamentId}/groups/{groupId}
// Test group update with ID not found - validate HTTP 404 response
TEST_F(GroupControllerTest, UpdateGroup_GroupNotFound_ReturnsHttp404) {
    std::string tournamentId = "12345678-1234-1234-1234-123456789abc";
    std::string groupId = "87654321-4321-4321-4321-123456789999";
    
    nlohmann::json requestJson = {
        {"name", "Non-existent Group"}
    };
    
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(
        testing::Eq(tournamentId),
        testing::Property(&domain::Group::Name, testing::Eq("Non-existent Group")),
        testing::Eq(groupId)
    )).WillOnce(testing::Throw(NotFoundException("Group not found for update")));
    
    crow::request request;
    request.body = requestJson.dump();
    
    crow::response response = groupController->UpdateGroup(request, tournamentId, groupId);
  
    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("Group not found for update", response.body);
}