#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Tournament.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
    public:
    MOCK_METHOD(std::string, CreateTournament, (std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (const std::string&), (override));
    MOCK_METHOD(void, Delete, (const std::string&), (override));
    MOCK_METHOD(std::string_view, SaveTournament, (const domain::Tournament&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, GetAllTournaments, (), (override));
    MOCK_METHOD(std::string_view, UpdateTournament, (const domain::Tournament&), (override));
};

class TournamentControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentDelegateMock> tournamentDelegateMock;
    std::shared_ptr<TournamentController> tournamentController;

    void SetUp() override {
        tournamentDelegateMock = std::make_shared<TournamentDelegateMock>();
        tournamentController = std::make_shared<TournamentController>(TournamentController(tournamentDelegateMock));
    }

    void TearDown() override {
    }
};

TEST_F(TournamentControllerTest, GetTournamentById_ErrorFormat) {
    crow::response badRequest = tournamentController->GetTournament("");

    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = tournamentController->GetTournament("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TournamentControllerTest, GetTournamentById) {
    std::shared_ptr<domain::Tournament> expectedTournament = std::make_shared<domain::Tournament>("Tournament Name");
    expectedTournament->Id() = "my-id";

    EXPECT_CALL(*tournamentDelegateMock, ReadById(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(expectedTournament));

    crow::response response = tournamentController->GetTournament("my-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTournament->Id(), jsonResponse["id"]);
    EXPECT_EQ(expectedTournament->Name(), jsonResponse["name"]);
}

TEST_F(TournamentControllerTest, GetTournamentNotFound) {
    EXPECT_CALL(*tournamentDelegateMock, ReadById(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = tournamentController->GetTournament("my-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TournamentControllerTest, DeleteTournament) {
    EXPECT_CALL(*tournamentDelegateMock, Delete(testing::Eq(std::string("my-id"))))
        .Times(1);

    crow::response response = tournamentController->DeleteTournament("my-id");

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
}

TEST_F(TournamentControllerTest, DeleteTournament_ErrorFormat) {
    crow::response badRequest = tournamentController->DeleteTournament("");

    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = tournamentController->DeleteTournament("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

//crear torneo valido HTTP 201
TEST_F(TournamentControllerTest, CreateTournament_ValidInput_Returns201) {
    auto tournament = std::make_shared<domain::Tournament>("Mundial 2025");

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(testing::_))
        .WillOnce(testing::Return("new-tournament-id"));

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
}

//buscar todos torneos con lista HTTP 200
TEST_F(TournamentControllerTest, ReadAll_WithTournaments_Returns200) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments = {
        std::make_shared<domain::Tournament>(domain::Tournament("Mundial 2025")),
        std::make_shared<domain::Tournament>(domain::Tournament("Copa America"))
    };

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    crow::response response = tournamentController->GetAllTournaments();

    EXPECT_EQ(crow::OK, response.code);
}

//actualizar torneo valido HTTP 204
TEST_F(TournamentControllerTest, UpdateTournament_ValidInput_Returns200) {
    // Primero simular que el torneo existe
    auto tournament = std::make_shared<domain::Tournament>("Mundial 2025");
    tournament->Id() = "tournament-id";

    EXPECT_CALL(*tournamentDelegateMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(testing::_))
        .WillOnce(testing::Return("tournament-id"));

    nlohmann::json tournamentBody = {{"name", "Mundial 2026"}};
    crow::request req;
    req.body = tournamentBody.dump();

    crow::response response = tournamentController->UpdateTournament("tournament-id", req);

    EXPECT_EQ(crow::OK, response.code);
}