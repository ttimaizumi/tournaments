#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
public:
  MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam,
              (const std::string_view id), (override));
  MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (),
              (override));
  MOCK_METHOD(std::string_view, CreateTeam, (const domain::Team&), (override));
  MOCK_METHOD(std::string_view, UpdateTeam, (const domain::Team&), (override));
  MOCK_METHOD(void, DeleteTeam, (const std::string_view id), (override));
};

class TeamControllerTest : public ::testing::Test {
protected:
  std::shared_ptr<TeamDelegateMock> teamDelegateMock;
  std::shared_ptr<TeamController> teamController;

  void SetUp() override {
    teamDelegateMock = std::make_shared<TeamDelegateMock>();
    teamController =
        std::make_shared<TeamController>(TeamController(teamDelegateMock));
  }
};

// GET /teams/{id}
TEST_F(TeamControllerTest, GetTeamById_ErrorFormat) {
  crow::response badRequest = teamController->getTeam("");

  EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

  badRequest = teamController->getTeam("mfasd#*");
  EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TeamControllerTest, GetTeamById) {
  std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id", "Team Name"});

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

// GET /teams
TEST_F(TeamControllerTest, GetAllTeams_ReturnsList) {
  std::vector<std::shared_ptr<domain::Team>> teams = {
    std::make_shared<domain::Team>(domain::Team{"id1", "Team One"}),
    std::make_shared<domain::Team>(domain::Team{"id2", "Team Two"})
  };

  EXPECT_CALL(*teamDelegateMock, GetAllTeams())
    .WillOnce(testing::Return(teams));

  crow::response response = teamController->getAllTeams();
  auto jsonResponse = crow::json::load(response.body);

  EXPECT_EQ(crow::OK, response.code);
  ASSERT_EQ(jsonResponse.size(), teams.size());
  EXPECT_EQ(jsonResponse[0]["id"], teams[0]->Id);
  EXPECT_EQ(jsonResponse[1]["name"], teams[1]->Name);
}

TEST_F(TeamControllerTest, GetAllTeams_ReturnsEmptyList) {
  EXPECT_CALL(*teamDelegateMock, GetAllTeams())
    .WillOnce(testing::Return(std::vector<std::shared_ptr<domain::Team>>{}));

  crow::response response = teamController->getAllTeams();
  auto jsonResponse = crow::json::load(response.body);

  EXPECT_EQ(crow::OK, response.code);
  ASSERT_EQ(jsonResponse.size(), 0);
}

// POST /teams
TEST_F(TeamControllerTest, CreateTeamTest) {
  domain::Team capturedTeam;
  EXPECT_CALL(*teamDelegateMock, CreateTeam(::testing::_))
    .WillOnce(testing::DoAll(testing::SaveArg<0>(&capturedTeam), testing::Return("new-id")));

  nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
  crow::request teamRequest;
  teamRequest.body = teamRequestBody.dump();

  crow::response response = teamController->createTeam(teamRequest);

  testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

  EXPECT_EQ(crow::CREATED, response.code);
  EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam.Id);
  EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

TEST_F(TeamControllerTest, CreateTeam_Conflict) {
  EXPECT_CALL(*teamDelegateMock, CreateTeam(::testing::_))
    .WillOnce(testing::Return("new-id"))
    .WillOnce(testing::Throw(std::runtime_error("duplicate key value")));
  
  nlohmann::json team1RequestBody = {{"id", "id1"}, {"name", "new team"}};
  crow::request teamRequest1;
  teamRequest1.body = team1RequestBody.dump();

  nlohmann::json team2RequestBody = {{"id", "id2"}, {"name", "new team"}};
  crow::request teamRequest2;
  teamRequest2.body = team2RequestBody.dump();

  crow::response response = teamController->createTeam(teamRequest1);
  crow::response conflictResponse = teamController->createTeam(teamRequest2);

  EXPECT_EQ(crow::CREATED, response.code);
  EXPECT_EQ(crow::CONFLICT, conflictResponse.code);
}

// PATCH /teams/{id}
TEST_F(TeamControllerTest, UpdateTeam_Success) {
  domain::Team capturedTeam;
  EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_))
    .WillOnce(testing::DoAll(testing::SaveArg<0>(&capturedTeam), testing::Return("id1")));

  nlohmann::json teamRequestBody = {{"name", "updated team"}};
  crow::request teamRequest;
  teamRequest.body = teamRequestBody.dump();

  crow::response response = teamController->updateTeam(teamRequest, "id1");

  EXPECT_EQ(crow::OK, response.code);
  EXPECT_EQ("id1", capturedTeam.Id);
  EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

TEST_F(TeamControllerTest, UpdateTeam_NotFound) {
  EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_))
    .WillOnce(testing::Return(""));

  nlohmann::json teamRequestBody = {{"name", "not found"}};
  crow::request teamRequest;
  teamRequest.body = teamRequestBody.dump();

  crow::response response = teamController->updateTeam(teamRequest, "id404");

  EXPECT_EQ(crow::NOT_FOUND, response.code);
}