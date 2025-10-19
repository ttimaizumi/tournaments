// teamcontrollertest.cpp — Pruebas mínimas esperadas para TeamController

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

using ::testing::_;
using ::testing::Eq;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;

// Mock del delegate que usa el controller
class TeamDelegateMock : public ITeamDelegate {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD(bool, UpdateTeam, (const domain::Team&), (override));
};

// Fixture
class TeamControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController   = std::make_shared<TeamController>(teamDelegateMock);
    }
};

/* =======================================================================================
 * 1) CREATE: JSON → Team, lo transferido al delegate es correcto, y HTTP 201
 * ======================================================================================= */
TEST_F(TeamControllerTest, CreateTeam_TransformsJson_TransfersToDelegate_Returns201) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamDelegateMock, SaveTeam(_))
        .WillOnce(DoAll(
            SaveArg<0>(&capturedTeam),
            Return(std::string_view{"new-id"})
        ));

    nlohmann::json body = {{"id","new-id"}, {"name","new team"}};
    crow::request req; req.body = body.dump();

    auto res = teamController->SaveTeam(req);

    // HTTP
    EXPECT_EQ(res.code, crow::CREATED);
    // Transferencia al delegate
    EXPECT_EQ(capturedTeam.Id,   "new-id");
    EXPECT_EQ(capturedTeam.Name, "new team");
    // (Opcional) Header location esperado
    EXPECT_EQ(res.get_header_value("location"), "new-id");
}

/* =======================================================================================
 * 2) CREATE: JSON → Team correcto, inserción falla en DB (delegate) → HTTP 409
 * ======================================================================================= */
TEST_F(TeamControllerTest, CreateTeam_TransformsJson_TransfersToDelegate_DBError_Returns409) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamDelegateMock, SaveTeam(_))
        .WillOnce(DoAll(
            SaveArg<0>(&capturedTeam),
            Return(std::string_view{}) // simulación fallo inserción
        ));

    nlohmann::json body = {{"id","dup"}, {"name","already"}};
    crow::request req; req.body = body.dump();

    auto res = teamController->SaveTeam(req);

    // HTTP
    EXPECT_EQ(res.code, crow::CONFLICT);
    // Transferencia al delegate
    EXPECT_EQ(capturedTeam.Id,   "dup");
    EXPECT_EQ(capturedTeam.Name, "already");
}

/* =======================================================================================
 * 3) READ BY ID: el ID transferido al delegate es el esperado; devuelve objeto → HTTP 200
 * ======================================================================================= */
TEST_F(TeamControllerTest, ReadById_TransfersIdToDelegate_Found_Returns200) {
    auto expected = std::make_shared<domain::Team>(domain::Team{"my-id","Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(Eq(std::string("my-id"))))
        .WillOnce(Return(expected));

    auto res = teamController->getTeam("my-id");
    EXPECT_EQ(res.code, crow::OK);

    // Validamos cuerpo JSON
    auto json = nlohmann::json::parse(res.body);
    EXPECT_EQ(json.at("id").get<std::string>(),   "my-id");
    EXPECT_EQ(json.at("name").get<std::string>(), "Team Name");
}

/* =======================================================================================
 * 4) READ BY ID: el ID transferido al delegate es el esperado; resultado nulo → HTTP 404
 * ======================================================================================= */
TEST_F(TeamControllerTest, ReadById_TransfersIdToDelegate_Null_Returns404) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(Eq(std::string("missing"))))
        .WillOnce(Return(nullptr));

    auto res = teamController->getTeam("missing");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/* =======================================================================================
 * 5) READ ALL: lista de objetos → HTTP 200
 * ======================================================================================= */
TEST_F(TeamControllerTest, ReadAll_ListWithItems_Returns200) {
    auto t1 = std::make_shared<domain::Team>(domain::Team{"a","A"});
    auto t2 = std::make_shared<domain::Team>(domain::Team{"b","B"});

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{t1, t2}));

    auto res = teamController->getAllTeams();
    EXPECT_EQ(res.code, crow::OK);

    // Validamos arreglo JSON
    auto json = nlohmann::json::parse(res.body);
    ASSERT_TRUE(json.is_array());
    ASSERT_EQ(json.size(), 2u);
    EXPECT_EQ(json[0].at("id").get<std::string>(),   "a");
    EXPECT_EQ(json[0].at("name").get<std::string>(), "A");
    EXPECT_EQ(json[1].at("id").get<std::string>(),   "b");
    EXPECT_EQ(json[1].at("name").get<std::string>(), "B");
}

/* =======================================================================================
 * 6) READ ALL: lista vacía → HTTP 200
 * ======================================================================================= */
TEST_F(TeamControllerTest, ReadAll_EmptyList_Returns200) {
    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

    auto res = teamController->getAllTeams();
    EXPECT_EQ(res.code, crow::OK);

    auto json = nlohmann::json::parse(res.body);
    ASSERT_TRUE(json.is_array());
    EXPECT_TRUE(json.empty());
}

/* =======================================================================================
 * 7) UPDATE: JSON → Team, lo transferido al delegate es correcto → HTTP 204
 *    (El controller debe usar el ID de la ruta, no el del body)
 * ======================================================================================= */
TEST_F(TeamControllerTest, UpdateTeam_TransformsJson_TransfersToDelegate_Returns204) {
    domain::Team captured;

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(_))
        .WillOnce(DoAll(
            SaveArg<0>(&captured),
            Return(true)
        ));

    // Body trae un id diferente, el controller DEBE sobrescribirlo con el id de ruta
    nlohmann::json body = {{"id","DIFFERENT_FROM_ROUTE"}, {"name","updated"}};
    crow::request req; req.body = body.dump();

    auto res = teamController->UpdateTeam(req, "route-id-123");
    EXPECT_EQ(res.code, crow::NO_CONTENT);

    // Transferencia al delegate
    EXPECT_EQ(captured.Id,   "route-id-123");
    EXPECT_EQ(captured.Name, "updated");
}

/* =======================================================================================
 * 8) UPDATE: JSON → Team correcto, delegate dice "no existe id" → HTTP 404
 * ======================================================================================= */
TEST_F(TeamControllerTest, UpdateTeam_TransformsJson_TransfersToDelegate_NotFound_Returns404) {
    domain::Team captured;

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(_))
        .WillOnce(DoAll(
            SaveArg<0>(&captured),
            Return(false) // simulación: no se encontró el ID a actualizar
        ));

    nlohmann::json body = {{"id","whatever"}, {"name","n/a"}};
    crow::request req; req.body = body.dump();

    auto res = teamController->UpdateTeam(req, "missing-id");
    EXPECT_EQ(res.code, crow::NOT_FOUND);

    // Transferencia al delegate
    EXPECT_EQ(captured.Id,   "missing-id"); // se usa el ID de la ruta
    EXPECT_EQ(captured.Name, "n/a");
}
