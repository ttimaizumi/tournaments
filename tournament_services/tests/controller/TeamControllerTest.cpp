#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
    public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD(std::string_view, UpdateTeam, (const domain::Team&), (override));
};

class TeamControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController = std::make_shared<TeamController>(TeamController(teamDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(TeamControllerTest, GetTeamById_ErrorFormat) {
    crow::response badRequest = teamController->getTeam("");

    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = teamController->getTeam("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TeamControllerTest, GetTeamById) {

    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id",  "Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam("my-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id, jsonResponse["id"]);
    EXPECT_EQ(expectedTeam->Name, jsonResponse["name"]);
}

TEST_F(TeamControllerTest, GetTeamNotFound) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = teamController->getTeam("my-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TeamControllerTest, SaveTeamTest) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Return("new-id")
            )
        );

    nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

TEST_F(TeamControllerTest, UpdateTeamTest) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Return("updated-id")
            )
        );

    nlohmann::json teamRequestBody = {{"name", "updated team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam("updated-id", teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("updated-id", capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

TEST_F(TeamControllerTest, UpdateTeam_ErrorFormat) {
    crow::response badRequest = teamController->UpdateTeam("", crow::request{});

    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = teamController->UpdateTeam("mfasd#*", crow::request{});
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}