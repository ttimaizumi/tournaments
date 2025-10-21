#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Tournament.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"
#include <nlohmann/json.hpp>

class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateTournament, (std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>), ReadAll, (), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Tournament>, std::string>), ReadById, (const std::string&), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), UpdateTournament, (const domain::Tournament&), (override));
};

class TournamentControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentDelegateMock> tournamentDelegateMock;
    std::shared_ptr<TournamentController> tournamentController;

    void SetUp() override {
        tournamentDelegateMock = std::make_shared<TournamentDelegateMock>();
        tournamentController = std::make_shared<TournamentController>(tournamentDelegateMock);
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(tournamentDelegateMock.get());
    }
};

// Test 1: Crear torneo válido - retorna HTTP 201 con Location header
TEST_F(TournamentControllerTest, CreateTournament_ValidInput_Returns201WithLocation) {
    const std::string expectedId = "new-tournament-id-123";

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(testing::_))
            .WillOnce(testing::Return(std::expected<std::string, std::string>(expectedId)));

    nlohmann::json tournamentBody = {
            {"name", "Mundial 2025"},
            {"format", {
                             {"numberOfGroups", 8},
                             {"maxTeamsPerGroup", 4},
                             {"type", "MUNDIAL"}
                     }}
    };
    crow::request req;
    req.body = tournamentBody.dump();

    crow::response response = tournamentController->CreateTournament(req);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(expectedId, response.get_header_value("location"));
}

// Test 2: Crear torneo duplicado - retorna HTTP 409 Conflict
TEST_F(TournamentControllerTest, CreateTournament_DuplicateName_Returns409Conflict) {
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(testing::_))
            .WillOnce(testing::Return(std::unexpected<std::string>("Tournament already exists")));

    nlohmann::json tournamentBody = {
            {"name", "Existing Tournament"},
            {"format", {
                             {"numberOfGroups", 4},
                             {"maxTeamsPerGroup", 4},
                             {"type", "LEAGUE"}
                     }}
    };
    crow::request req;
    req.body = tournamentBody.dump();

    crow::response response = tournamentController->CreateTournament(req);

    EXPECT_EQ(crow::CONFLICT, response.code);
}

// Test 3: Crear torneo - validar transformación JSON a objeto Tournament
TEST_F(TournamentControllerTest, CreateTournament_TransformsJsonCorrectly) {
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournament),
                    testing::Return(std::expected<std::string, std::string>("new-id"))
            ));

    nlohmann::json tournamentBody = {
            {"name", "Copa America 2025"},
            {"format", {
                             {"numberOfGroups", 4},
                             {"maxTeamsPerGroup", 4},
                             {"type", "CUP"}
                     }}
    };
    crow::request req;
    req.body = tournamentBody.dump();

    tournamentController->CreateTournament(req);

    ASSERT_NE(nullptr, capturedTournament);
    EXPECT_EQ("Copa America 2025", capturedTournament->Name());
}

// Test 4: Crear torneo - JSON inválido retorna HTTP 400
TEST_F(TournamentControllerTest, CreateTournament_InvalidJson_ReturnsBadRequest) {
    crow::request req;
    req.body = "invalid json {{{";

    crow::response response = tournamentController->CreateTournament(req);

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

// Test 5: Crear torneo - body vacío retorna HTTP 400
TEST_F(TournamentControllerTest, CreateTournament_EmptyBody_ReturnsBadRequest) {
    crow::request req;
    req.body = "";

    crow::response response = tournamentController->CreateTournament(req);

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

// Test 6: Buscar torneo por ID - formato de ID inválido retorna HTTP 400
TEST_F(TournamentControllerTest, GetTournamentById_InvalidIdFormat_ReturnsBadRequest) {
    crow::response response = tournamentController->GetTournament("");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);

    response = tournamentController->GetTournament("mfasd#*");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);

    response = tournamentController->GetTournament("invalid id with spaces");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

// Test 7: Buscar torneo por ID - retorna HTTP 200 con objeto Tournament
TEST_F(TournamentControllerTest, GetTournamentById_ValidId_Returns200WithTournament) {
    std::shared_ptr<domain::Tournament> expectedTournament =
            std::make_shared<domain::Tournament>("Mundial 2026");
    expectedTournament->Id() = "tournament-id-456";

    EXPECT_CALL(*tournamentDelegateMock, ReadById(testing::Eq(std::string("tournament-id-456"))))
            .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(expectedTournament)));

    crow::response response = tournamentController->GetTournament("tournament-id-456");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("tournament-id-456", jsonResponse["id"].get<std::string>());
    EXPECT_EQ("Mundial 2026", jsonResponse["name"].get<std::string>());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

// Test 8: Buscar torneo por ID - torneo no encontrado retorna HTTP 404
TEST_F(TournamentControllerTest, GetTournamentById_NotFound_Returns404) {
    EXPECT_CALL(*tournamentDelegateMock, ReadById(testing::Eq(std::string("non-existent-id"))))
            .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(nullptr)));

    crow::response response = tournamentController->GetTournament("non-existent-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test 9: Buscar torneo por ID - valida que se transfiere ID correcto al delegate
TEST_F(TournamentControllerTest, GetTournamentById_TransfersCorrectIdToDelegate) {
    std::string capturedId;
    auto mockTournament = std::make_shared<domain::Tournament>("Test Tournament");
    mockTournament->Id() = "test-id";

    EXPECT_CALL(*tournamentDelegateMock, ReadById(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedId),
                    testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(mockTournament))
            ));

    tournamentController->GetTournament("specific-id-789");

    EXPECT_EQ("specific-id-789", capturedId);
}

// Test 10: Buscar todos los torneos - lista con objetos retorna HTTP 200
TEST_F(TournamentControllerTest, GetAllTournaments_WithTournaments_Returns200WithArray) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments = {
            std::make_shared<domain::Tournament>("Mundial 2025"),
            std::make_shared<domain::Tournament>("Copa America"),
            std::make_shared<domain::Tournament>("Champions League")
    };
    tournaments[0]->Id() = "id-1";
    tournaments[1]->Id() = "id-2";
    tournaments[2]->Id() = "id-3";

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
            .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>(tournaments)));

    crow::response response = tournamentController->ReadAll();
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(3, jsonResponse.size());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

// Test 11: Buscar todos los torneos - lista vacía retorna HTTP 200 con array vacío
TEST_F(TournamentControllerTest, GetAllTournaments_EmptyList_Returns200WithEmptyArray) {
    std::vector<std::shared_ptr<domain::Tournament>> emptyTournaments;

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
            .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>(emptyTournaments)));

    crow::response response = tournamentController->ReadAll();
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(0, jsonResponse.size());
    EXPECT_TRUE(jsonResponse.is_array());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

// Test 12: Actualizar torneo - actualización exitosa retorna HTTP 204
TEST_F(TournamentControllerTest, UpdateTournament_ValidInput_Returns204NoContent) {
    domain::Tournament capturedTournament("Temp");
    const std::string tournamentId = "tournament-id-update";

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournament),
                    testing::Return(std::expected<std::string, std::string>(tournamentId))
            ));

    nlohmann::json tournamentBody = {{"name", "Mundial 2026 Updated"}};
    crow::request req;
    req.body = tournamentBody.dump();

    crow::response response = tournamentController->UpdateTournament(req, tournamentId);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
    EXPECT_EQ(tournamentId, capturedTournament.Id());
    EXPECT_EQ("Mundial 2026 Updated", capturedTournament.Name());
}

// Test 13: Actualizar torneo - torneo no encontrado retorna HTTP 404
TEST_F(TournamentControllerTest, UpdateTournament_TournamentNotFound_Returns404) {
    domain::Tournament capturedTournament("Temp");
    const std::string nonExistentId = "non-existent-tournament-id";

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournament),
                    testing::Return(std::unexpected<std::string>("Tournament not found"))
            ));

    nlohmann::json tournamentBody = {{"name", "Updated Tournament"}};
    crow::request req;
    req.body = tournamentBody.dump();

    crow::response response = tournamentController->UpdateTournament(req, nonExistentId);

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ(nonExistentId, capturedTournament.Id());
    EXPECT_EQ("Updated Tournament", capturedTournament.Name());
}

// Test 14: Actualizar torneo - valida transformación JSON a objeto Tournament
TEST_F(TournamentControllerTest, UpdateTournament_TransformsJsonCorrectly) {
    domain::Tournament capturedTournament("Temp");
    const std::string tournamentId = "transform-test-id";

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournament),
                    testing::Return(std::expected<std::string, std::string>(tournamentId))
            ));

    nlohmann::json tournamentBody = {
            {"name", "Transformed Tournament Name"}
    };
    crow::request req;
    req.body = tournamentBody.dump();

    tournamentController->UpdateTournament(req, tournamentId);

    EXPECT_EQ(tournamentId, capturedTournament.Id());
    EXPECT_EQ("Transformed Tournament Name", capturedTournament.Name());
}

// Test 15: Actualizar torneo - formato de ID inválido retorna HTTP 400
TEST_F(TournamentControllerTest, UpdateTournament_InvalidIdFormat_ReturnsBadRequest) {
    crow::request req;
    nlohmann::json tournamentBody = {{"name", "Tournament"}};
    req.body = tournamentBody.dump();

    crow::response response = tournamentController->UpdateTournament(req, "");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);

    response = tournamentController->UpdateTournament(req, "invalid#id*");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);

    response = tournamentController->UpdateTournament(req, "id with spaces");
    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

// Test 16: Actualizar torneo - JSON inválido retorna HTTP 400
TEST_F(TournamentControllerTest, UpdateTournament_InvalidJson_ReturnsBadRequest) {
    crow::request req;
    req.body = "not valid json";

    crow::response response = tournamentController->UpdateTournament(req, "valid-id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

// Test 17: Actualizar torneo - body vacío retorna HTTP 400
TEST_F(TournamentControllerTest, UpdateTournament_EmptyBody_ReturnsBadRequest) {
    crow::request req;
    req.body = "";

    crow::response response = tournamentController->UpdateTournament(req, "valid-id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}
