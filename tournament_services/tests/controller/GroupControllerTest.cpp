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
        groupController = std::make_shared<GroupController>(groupDelegateMock);
    }
};

// GET /tournaments/{tournamentId}/groups
// Good path No teams - 200
TEST_F(GroupControllerTest, GetGroups) {
    std::shared_ptr<domain::Group> expectedGroup = std::make_shared<domain::Group>(domain::Group{"Group Name", "group-id"});
    expectedGroup->TournamentId() = "tournament-id";

    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq(std::string("tournament-id"))))
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>{std::vector<std::shared_ptr<domain::Group>>{expectedGroup}}));

    crow::response response = groupController->GetGroups("tournament-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    ASSERT_EQ(jsonResponse.size(), 1);
}

// Good path With teams - 200
TEST_F(GroupControllerTest, GetGroupsWithTeams) {
    domain::Team team1{"team-1", "Team One"};
    domain::Team team2{"team-2", "Team Two"};
    
    std::shared_ptr<domain::Group> expectedGroup = std::make_shared<domain::Group>(domain::Group{"Group Name", "group-id"});
    expectedGroup->TournamentId() = "tournament-id";
    expectedGroup->Teams().push_back(team1);
    expectedGroup->Teams().push_back(team2);

    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq(std::string("tournament-id"))))
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>{std::vector<std::shared_ptr<domain::Group>>{expectedGroup}}));

    crow::response response = groupController->GetGroups("tournament-id");
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
    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq(std::string("nonexistent-tournament"))))
        .WillOnce(testing::Throw(NotFoundException("Tournament not found")));

    crow::response response = groupController->GetGroups("nonexistent-tournament");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("Tournament not found", response.body);
}

// Invalid format tournamentId - 400
TEST_F(GroupControllerTest, GetGroupsInvalidTournamentIdFormat) {
    crow::response response = groupController->GetGroups("");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid tournament ID format", response.body);

    response = groupController->GetGroups("invalid@tournament#id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid tournament ID format", response.body);
}