#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
public:
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Team>, std::string>), GetTeam, (const std::string_view id), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string>), GetAllTeams, (), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), UpdateTeam, (const domain::Team&), (override));
};

class TeamControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController = std::make_shared<TeamController>(teamDelegateMock);
    }

    void TearDown() override {

    }
};

// get team by id tests

TEST_F(TeamControllerTest, GetTeamById_InvalidFormat_ReturnsBadRequest) {
    crow::response response = teamController->getTeam("");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);

    response = teamController->getTeam("mfasd#*");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);

    response = teamController->getTeam("invalid id");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

TEST_F(TeamControllerTest, GetTeamById_ValidId_Returns200WithTeam) {
    std::shared_ptr<domain::Team> expectedTeam =
            std::make_shared<domain::Team>(domain::Team{"my-id", "Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
            .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Team>, std::string>(expectedTeam)));

    crow::response response = teamController->getTeam("my-id");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id, jsonResponse["id"].get<std::string>());
    EXPECT_EQ(expectedTeam->Name, jsonResponse["name"].get<std::string>());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

TEST_F(TeamControllerTest, GetTeamById_NotFound_Returns404) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("non-existent-id"))))
            .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Team>, std::string>(nullptr)));

    crow::response response = teamController->getTeam("non-existent-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// get all team tests

TEST_F(TeamControllerTest, GetAllTeams_WithMultipleTeams_Returns200WithArray) {
    std::vector<std::shared_ptr<domain::Team>> teams = {
            std::make_shared<domain::Team>(domain::Team{"id-1", "Team 1"}),
            std::make_shared<domain::Team>(domain::Team{"id-2", "Team 2"}),
            std::make_shared<domain::Team>(domain::Team{"id-3", "Team 3"})
    };

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
            .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string>(teams)));

    crow::response response = teamController->getAllTeams();
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(3, jsonResponse.size());
    EXPECT_EQ("id-1", jsonResponse[0]["id"].get<std::string>());
    EXPECT_EQ("Team 1", jsonResponse[0]["name"].get<std::string>());
    EXPECT_EQ("id-2", jsonResponse[1]["id"].get<std::string>());
    EXPECT_EQ("Team 2", jsonResponse[1]["name"].get<std::string>());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

TEST_F(TeamControllerTest, GetAllTeams_EmptyList_Returns200WithEmptyArray) {
    std::vector<std::shared_ptr<domain::Team>> emptyTeams;

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
            .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string>(emptyTeams)));

    crow::response response = teamController->getAllTeams();
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(0, jsonResponse.size());
    EXPECT_TRUE(jsonResponse.is_array());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

// save team tests

TEST_F(TeamControllerTest, SaveTeam_ValidRequest_Returns201WithLocation) {
    domain::Team capturedTeam;
    const std::string expectedId = "generated-id-12345";

    EXPECT_CALL(*teamDelegateMock, SaveTeam(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTeam),
                    testing::Return(std::expected<std::string, std::string>(expectedId))
            ));

    nlohmann::json teamRequestBody = {{"name", "New Team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ("New Team", capturedTeam.Name);
    EXPECT_EQ(expectedId, response.get_header_value("location"));

    testing::Mock::VerifyAndClearExpectations(teamDelegateMock.get());
}

TEST_F(TeamControllerTest, SaveTeam_DuplicateName_Returns409Conflict) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamDelegateMock, SaveTeam(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTeam),
                    testing::Return(std::unexpected<std::string>("Team already exists"))
            ));

    nlohmann::json teamRequestBody = {{"name", "Existing Team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    EXPECT_EQ(crow::CONFLICT, response.code);
    EXPECT_EQ("Existing Team", capturedTeam.Name);

    testing::Mock::VerifyAndClearExpectations(teamDelegateMock.get());
}

TEST_F(TeamControllerTest, SaveTeam_EmptyBody_ReturnsBadRequest) {
    crow::request teamRequest;
    teamRequest.body = "";

    crow::response response = teamController->SaveTeam(teamRequest);

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

// update team tests

TEST_F(TeamControllerTest, UpdateTeam_ValidRequest_Returns204NoContent) {
    domain::Team capturedTeam;
    const std::string teamId = "existing-id-123";

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTeam),
                    testing::Return(std::expected<std::string, std::string>(teamId))
            ));

    nlohmann::json teamRequestBody = {{"name", "Updated Team Name"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, teamId);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
    EXPECT_EQ(teamId, capturedTeam.Id);
    EXPECT_EQ("Updated Team Name", capturedTeam.Name);

    testing::Mock::VerifyAndClearExpectations(teamDelegateMock.get());
}

TEST_F(TeamControllerTest, UpdateTeam_TeamNotFound_Returns404) {
    domain::Team capturedTeam;
    const std::string nonExistentId = "non-existent-id";

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTeam),
                    testing::Return(std::unexpected<std::string>("Team not found"))
            ));

    nlohmann::json teamRequestBody = {{"name", "Updated Team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, nonExistentId);

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ(nonExistentId, capturedTeam.Id);
    EXPECT_EQ("Updated Team", capturedTeam.Name);

    testing::Mock::VerifyAndClearExpectations(teamDelegateMock.get());
}

TEST_F(TeamControllerTest, UpdateTeam_InvalidIdFormat_ReturnsBadRequest) {
    crow::request teamRequest;
    nlohmann::json teamRequestBody = {{"name", "Team"}};
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, "");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);

    response = teamController->UpdateTeam(teamRequest, "invalid#id*");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);

    response = teamController->UpdateTeam(teamRequest, "id with spaces");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

TEST_F(TeamControllerTest, UpdateTeam_InvalidJSON_ReturnsBadRequest) {
    crow::request teamRequest;
    teamRequest.body = "not valid json";

    crow::response response = teamController->UpdateTeam(teamRequest, "valid-id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

TEST_F(TeamControllerTest, UpdateTeam_EmptyBody_ReturnsBadRequest) {
    crow::request teamRequest;
    teamRequest.body = "";

    crow::response response = teamController->UpdateTeam(teamRequest, "valid-id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}
