#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD(bool, UpdateTeam, (const domain::Team&), (override));
};

class TeamControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController = std::make_shared<TeamController>(TeamController(teamDelegateMock));
    }
};

TEST_F(TeamControllerTest, GetTeamById_ErrorFormat) {
    crow::response badRequest = teamController->getTeam("");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = teamController->getTeam("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}


// Buscar equipo por ID -> delegate recibe el ID esperado -> retorna objeto -> HTTP 200
TEST_F(TeamControllerTest, GetTeamById) {
    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id",  "Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam("my-id");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id,   jsonResponse.at("id").get<std::string>());
    EXPECT_EQ(expectedTeam->Name, jsonResponse.at("name").get<std::string>());
}



// Buscar equipo por ID -> delegate recibe el ID esperado -> retorna nullptr -> HTTP 404
TEST_F(TeamControllerTest, GetTeamNotFound) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = teamController->getTeam("my-id");
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}


// Crear equipo -> transformar JSON a Team, pasar al delegate, delegate regresa id -> HTTP 201
TEST_F(TeamControllerTest, SaveTeam_Created_201) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTeam),
            testing::Return(std::string_view{"new-id"})
        ));

    nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(),   capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}


// Crear equipo con error -> transformar JSON a Team, pasar al delegate,
// simular insercion fallida (id vacio) -> HTTP 409
TEST_F(TeamControllerTest, SaveTeam_Conflict_409) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTeam),
            testing::Return(std::string_view{}) // simulamos inserción fallida
        ));

    nlohmann::json body = {{"id","dup"},{"name","already"}};
    crow::request req; req.body = body.dump();

    auto res = teamController->SaveTeam(req);

    EXPECT_EQ(res.code, crow::CONFLICT);
    EXPECT_EQ(capturedTeam.Id,   "dup");
    EXPECT_EQ(capturedTeam.Name, "already");
}



// Buscar equipos -> delegate retorna lista con elementos -> HTTP 200 y JSON con 2 items
TEST_F(TeamControllerTest, GetAllTeams_WithItems_200) {
    auto t1 = std::make_shared<domain::Team>(domain::Team{"a","A"});
    auto t2 = std::make_shared<domain::Team>(domain::Team{"b","B"});

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(::testing::Return(std::vector<std::shared_ptr<domain::Team>>{t1, t2}));

    auto res = teamController->getAllTeams();
    EXPECT_EQ(res.code, crow::OK);

    auto json = nlohmann::json::parse(res.body);
    ASSERT_TRUE(json.is_array());
    ASSERT_EQ(json.size(), 2);
    EXPECT_EQ(json[0].at("id").get<std::string>(),   "a");
    EXPECT_EQ(json[0].at("name").get<std::string>(), "A");
    EXPECT_EQ(json[1].at("id").get<std::string>(),   "b");
    EXPECT_EQ(json[1].at("name").get<std::string>(), "B");
}


// Buscar equipos -> delegate retorna lista vacia -> HTTP 200 y JSON []
TEST_F(TeamControllerTest, GetAllTeams_Empty_200) {
    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(::testing::Return(std::vector<std::shared_ptr<domain::Team>>{}));

    auto res = teamController->getAllTeams();
    EXPECT_EQ(res.code, crow::OK);

    auto json = nlohmann::json::parse(res.body);
    ASSERT_TRUE(json.is_array());
    EXPECT_TRUE(json.empty());
}



// Actualizar equipo -> transformar JSON a Team, pasar al delegate,
// delegate retorna true (encontro y actualizo) -> HTTP 204
TEST_F(TeamControllerTest, UpdateTeam_NoContent_204) {
    domain::Team captured{};
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&captured),
            ::testing::Return(true)
        ));

    nlohmann::json body = {{"id","u1"},{"name","updated"}};
    crow::request req; req.body = body.dump();

    auto res = teamController->UpdateTeam(req);

    EXPECT_EQ(res.code, crow::NO_CONTENT);
    EXPECT_EQ(captured.Id,   "u1");
    EXPECT_EQ(captured.Name, "updated");
}


// Actualizar equipo -> transformar JSON a Team, pasar al delegate,
// delegate retorna false (no encontro ID) -> HTTP 404
TEST_F(TeamControllerTest, UpdateTeam_NotFound_404) {
    domain::Team captured{};
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&captured),
            ::testing::Return(false)
        ));

    nlohmann::json body = {{"id","missing"},{"name","whatever"}};
    crow::request req; req.body = body.dump();

    auto res = teamController->UpdateTeam(req);

    EXPECT_EQ(res.code, crow::NOT_FOUND);
    EXPECT_EQ(captured.Id,   "missing");
    EXPECT_EQ(captured.Name, "whatever");
}