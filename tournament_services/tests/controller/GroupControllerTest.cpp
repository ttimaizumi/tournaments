#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "controller/GroupController.hpp"

class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateGroup, (const std::string_view& tournamentId, const domain::Group& group), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>), GetGroups, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>), GetGroup, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateGroup, (const std::string_view& tournamentId, const domain::Group& group), (override));
    MOCK_METHOD((std::expected<void, std::string>), RemoveGroup, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateTeams, (const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams), (override));
};

class GroupControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<GroupDelegateMock> groupDelegateMock;
    std::shared_ptr<GroupController> groupController;

    void SetUp() override {
        groupDelegateMock = std::make_shared<GroupDelegateMock>();
        groupController = std::make_shared<GroupController>(GroupController(groupDelegateMock));
    }

    void TearDown() override {
    }
};

// Test CreateGroup - success (HTTP 201)
TEST_F(GroupControllerTest, CreateGroup_Success) {
    domain::Group capturedGroup;

    EXPECT_CALL(*groupDelegateMock, CreateGroup(testing::Eq("tournament-1"), testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<1>(&capturedGroup),
            testing::Return(std::expected<std::string, std::string>("group-id-123"))
        ));

    nlohmann::json groupRequestBody = {
        {"name", "Group A"},
        {"teams", nlohmann::json::array()}
    };
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();

    crow::response response = groupController->CreateGroup(groupRequest, "tournament-1");

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ("Group A", capturedGroup.Name());
    EXPECT_EQ("group-id-123", response.get_header_value("location"));
}

// Test CreateGroup - error duplicate (HTTP 422)
TEST_F(GroupControllerTest, CreateGroup_Error) {
    EXPECT_CALL(*groupDelegateMock, CreateGroup(testing::Eq("tournament-1"), testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group already exists")));

    nlohmann::json groupRequestBody = {
        {"name", "Group A"},
        {"teams", nlohmann::json::array()}
    };
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();

    crow::response response = groupController->CreateGroup(groupRequest, "tournament-1");

    EXPECT_EQ(422, response.code);
}

// Test GetGroup - success (HTTP 200)
TEST_F(GroupControllerTest, GetGroup_Success) {
    auto expectedGroup = std::make_shared<domain::Group>("Group A", "group-1");
    expectedGroup->TournamentId() = "tournament-1";

    EXPECT_CALL(*groupDelegateMock, GetGroup(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(expectedGroup)));

    crow::response response = groupController->GetGroup("tournament-1", "group-1");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("Group A", jsonResponse["name"].s());
    EXPECT_EQ("group-1", jsonResponse["id"].s());
}

// Test GetGroup - not found (HTTP 404)
TEST_F(GroupControllerTest, GetGroup_NotFound) {
    EXPECT_CALL(*groupDelegateMock, GetGroup(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group not found")));

    crow::response response = groupController->GetGroup("tournament-1", "group-1");

    EXPECT_EQ(crow::INTERNAL_SERVER_ERROR, response.code);
}

// Test GetGroups - success with list (HTTP 200)
TEST_F(GroupControllerTest, GetGroups_Success) {
    std::vector<std::shared_ptr<domain::Group>> groups;
    auto group1 = std::make_shared<domain::Group>("Group A", "group-1");
    group1->TournamentId() = "tournament-1";
    auto group2 = std::make_shared<domain::Group>("Group B", "group-2");
    group2->TournamentId() = "tournament-1";
    groups.push_back(group1);
    groups.push_back(group2);

    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups)));

    crow::response response = groupController->GetGroups("tournament-1");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(2, jsonResponse.size());
}

// Test GetGroups - empty list (HTTP 200)
TEST_F(GroupControllerTest, GetGroups_EmptyList) {
    std::vector<std::shared_ptr<domain::Group>> emptyGroups;

    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(emptyGroups)));

    crow::response response = groupController->GetGroups("tournament-1");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(0, jsonResponse.size());
}

// Test UpdateGroup - success (HTTP 204)
TEST_F(GroupControllerTest, UpdateGroup_Success) {
    domain::Group capturedGroup;

    EXPECT_CALL(*groupDelegateMock, UpdateGroup(testing::Eq("tournament-1"), testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<1>(&capturedGroup),
            testing::Return(std::expected<void, std::string>())
        ));

    nlohmann::json groupRequestBody = {{"name", "Updated Group A"}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();

    crow::response response = groupController->UpdateGroup(groupRequest, "tournament-1", "group-1");

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
    EXPECT_EQ("Updated Group A", capturedGroup.Name());
    EXPECT_EQ("group-1", capturedGroup.Id());
}

// Test UpdateGroup - not found (HTTP 404)
TEST_F(GroupControllerTest, UpdateGroup_NotFound) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(testing::Eq("tournament-1"), testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group doesn't exist")));

    nlohmann::json groupRequestBody = {{"name", "Updated Group A"}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();

    crow::response response = groupController->UpdateGroup(groupRequest, "tournament-1", "group-1");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test UpdateTeams - success (HTTP 204)
TEST_F(GroupControllerTest, UpdateTeams_Success) {
    std::vector<domain::Team> capturedTeams;

    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-1"), testing::Eq("group-1"), testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<2>(&capturedTeams),
            testing::Return(std::expected<void, std::string>())
        ));

    nlohmann::json teamsRequestBody = nlohmann::json::array();
    teamsRequestBody.push_back({{"id", "team-1"}, {"name", "Team A"}});
    teamsRequestBody.push_back({{"id", "team-2"}, {"name", "Team B"}});

    crow::request teamsRequest;
    teamsRequest.body = teamsRequestBody.dump();

    crow::response response = groupController->UpdateTeams(teamsRequest, "tournament-1", "group-1");

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
    EXPECT_EQ(2, capturedTeams.size());
    EXPECT_EQ("team-1", capturedTeams[0].Id);
    EXPECT_EQ("Team A", capturedTeams[0].Name);
}

// Test UpdateTeams - team doesn't exist (HTTP 422)
TEST_F(GroupControllerTest, UpdateTeams_TeamDoesNotExist) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-1"), testing::Eq("group-1"), testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team team-1 doesn't exist")));

    nlohmann::json teamsRequestBody = nlohmann::json::array();
    teamsRequestBody.push_back({{"id", "team-1"}, {"name", "Team A"}});

    crow::request teamsRequest;
    teamsRequest.body = teamsRequestBody.dump();

    crow::response response = groupController->UpdateTeams(teamsRequest, "tournament-1", "group-1");

    EXPECT_EQ(422, response.code);
}

// Test UpdateTeams - group is full (HTTP 422)
TEST_F(GroupControllerTest, UpdateTeams_GroupFull) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-1"), testing::Eq("group-1"), testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group at max capacity")));

    nlohmann::json teamsRequestBody = nlohmann::json::array();
    teamsRequestBody.push_back({{"id", "team-1"}, {"name", "Team A"}});

    crow::request teamsRequest;
    teamsRequest.body = teamsRequestBody.dump();

    crow::response response = groupController->UpdateTeams(teamsRequest, "tournament-1", "group-1");

    EXPECT_EQ(422, response.code);
}

// Test RemoveGroup - success (HTTP 204)
TEST_F(GroupControllerTest, RemoveGroup_Success) {
    EXPECT_CALL(*groupDelegateMock, RemoveGroup(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(std::expected<void, std::string>()));

    crow::response response = groupController->RemoveGroup("tournament-1", "group-1");

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
}

// Test RemoveGroup - group not found (HTTP 404)
TEST_F(GroupControllerTest, RemoveGroup_GroupNotFound) {
    EXPECT_CALL(*groupDelegateMock, RemoveGroup(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group doesn't exist")));

    crow::response response = groupController->RemoveGroup("tournament-1", "group-1");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test RemoveGroup - tournament not found (HTTP 404)
TEST_F(GroupControllerTest, RemoveGroup_TournamentNotFound) {
    EXPECT_CALL(*groupDelegateMock, RemoveGroup(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(std::unexpected<std::string>("Tournament doesn't exist")));

    crow::response response = groupController->RemoveGroup("tournament-1", "group-1");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}
