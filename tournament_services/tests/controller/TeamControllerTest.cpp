#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <expected>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"
#include "exception/Error.hpp"

class TeamDelegateMock : public ITeamDelegate {
public:
  MOCK_METHOD((std::expected<std::shared_ptr<domain::Team>, Error>), GetTeam,
              (std::string_view id), (override));
  MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Team>>, Error>), GetAllTeams, (), (override));
  MOCK_METHOD((std::expected<std::string, Error>), CreateTeam, (const domain::Team&), (override));
  MOCK_METHOD((std::expected<std::string, Error>), UpdateTeam, (const domain::Team&), (override));
  MOCK_METHOD((std::expected<void, Error>), DeleteTeam, (std::string_view id), (override));
};

class TeamControllerTest : public ::testing::Test {
protected:
  std::shared_ptr<TeamDelegateMock> teamDelegateMock;
  std::shared_ptr<TeamController> teamController;

  void SetUp() override {
    teamDelegateMock = std::make_shared<TeamDelegateMock>();
    teamController = std::make_shared<TeamController>(teamDelegateMock);
  }
};

// Tests de CreateTeam

// Validacion del JSON y creacion exitosa. Response 201
TEST_F(TeamControllerTest, CreateTeam_ValidTeam) {
  domain::Team capturedTeam;
  EXPECT_CALL(*teamDelegateMock, CreateTeam(testing::_))
    .WillOnce(testing::DoAll(testing::SaveArg<0>(&capturedTeam),
                             testing::Return(std::expected<std::string, Error>{std::in_place, "new-id"})));

  nlohmann::json teamRequestBody = {{"name", "new team"}};
  crow::request teamRequest;
  teamRequest.body = teamRequestBody.dump();

  crow::response response = teamController->createTeam(teamRequest);

  EXPECT_EQ(crow::CREATED, response.code);
  EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

// Validacion del JSON y conflicto en DB. Response 409
TEST_F(TeamControllerTest, CreateTeam_DbConflict_Returns409) {
  EXPECT_CALL(*teamDelegateMock, CreateTeam(testing::_))
    .WillOnce(testing::Return(std::expected<std::string, Error>{std::unexpected(Error::DUPLICATE)}));

  nlohmann::json teamRequestBody = {{"name", "dupe team"}};
  crow::request teamRequest;
  teamRequest.body = teamRequestBody.dump();

  crow::response response = teamController->createTeam(teamRequest);

  EXPECT_EQ(crow::CONFLICT, response.code);
}

// Tests de GetTeam

// Validar respuesta exitosa y contenido. Response 200
TEST_F(TeamControllerTest, GetTeamById_Returns200AndBody) {
  auto expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id", "Team Name"});

  EXPECT_CALL(*teamDelegateMock, GetTeam(std::string_view("my-id")))
    .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Team>, Error>{std::in_place, expectedTeam}));

  crow::response response = teamController->getTeam("my-id");
  auto jsonResponse = crow::json::load(response.body);

  EXPECT_EQ(crow::OK, response.code);
  EXPECT_EQ(expectedTeam->Id, jsonResponse["id"]);
  EXPECT_EQ(expectedTeam->Name, jsonResponse["name"]);
}

// Validar respuesta NOT_FOUND. Response 404
TEST_F(TeamControllerTest, GetTeamById_NotFound_Returns404) {
  EXPECT_CALL(*teamDelegateMock, GetTeam(std::string_view("missing-id")))
      .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Team>, Error>{std::unexpected(Error::NOT_FOUND)}));

  crow::response response = teamController->getTeam("missing-id");
  EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Tests de GetAllTeams

// Validar respuesta exitosa con lista de equipos. Response 200
TEST_F(TeamControllerTest, GetAllTeams_ReturnsList200) {
  std::vector<std::shared_ptr<domain::Team>> teams = {
    std::make_shared<domain::Team>(domain::Team{"id1", "Team One"}),
    std::make_shared<domain::Team>(domain::Team{"id2", "Team Two"})
  };

  EXPECT_CALL(*teamDelegateMock, GetAllTeams())
    .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Team>>, Error>{std::in_place, teams}));

  crow::response response = teamController->getAllTeams();
  auto jsonResponse = crow::json::load(response.body);

  EXPECT_EQ(crow::OK, response.code);
  ASSERT_EQ(jsonResponse.size(), teams.size());
  EXPECT_EQ(jsonResponse[0]["id"], teams[0]->Id);
  EXPECT_EQ(jsonResponse[1]["name"], teams[1]->Name);
}

// Validar respuesta exitosa con lista vacía. Response 200
TEST_F(TeamControllerTest, GetAllTeams_ReturnsEmptyList200) {
  EXPECT_CALL(*teamDelegateMock, GetAllTeams())
    .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Team>>, Error>{std::in_place}));

  crow::response response = teamController->getAllTeams();
  auto jsonResponse = crow::json::load(response.body);

  EXPECT_EQ(crow::OK, response.code);
  ASSERT_EQ(jsonResponse.size(), 0);
}

// Tests de UpdateTeam

// Validacion del JSON y actualizacion exitosa. Response 200
TEST_F(TeamControllerTest, UpdateTeam_ValidJson_DelegatesAndReturns200) {
  domain::Team capturedTeam;
  EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::_))
    .WillOnce(testing::DoAll(testing::SaveArg<0>(&capturedTeam),
                             testing::Return(std::expected<std::string, Error>{std::in_place, "id1"})));

  nlohmann::json teamRequestBody = {{"name", "updated team"}};
  crow::request teamRequest;
  teamRequest.body = teamRequestBody.dump();

  crow::response response = teamController->updateTeam(teamRequest, "id1");

  EXPECT_EQ(crow::OK, response.code);
  EXPECT_EQ("id1", capturedTeam.Id);
  EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

// Validacion del JSON y equipo no encontrado. Response 404
TEST_F(TeamControllerTest, UpdateTeam_NotFound_Returns404) {
  EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::_))
    .WillOnce(testing::Return(std::expected<std::string, Error>{std::unexpected(Error::NOT_FOUND)}));

  nlohmann::json teamRequestBody = {{"name", "not found"}};
  crow::request teamRequest;
  teamRequest.body = teamRequestBody.dump();

  crow::response response = teamController->updateTeam(teamRequest, "id404");

  EXPECT_EQ(crow::NOT_FOUND, response.code);
}