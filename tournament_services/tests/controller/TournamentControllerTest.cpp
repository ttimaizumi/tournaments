#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>
#include "domain/Tournament.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
#include <memory.h>
// #include <gtest/gtest.h>
// #include "domain/Team.hpp"
// #include "controller/TournamentController.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, GetTournament,
                (std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::string, CreateTournament, (const domain::Tournament&), (override));
    MOCK_METHOD(std::string, UpdateTournament, (const domain::Tournament&), (override));
    MOCK_METHOD(void, DeleteTournament, (std::string_view id), (override));
};

class TournamentControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentDelegateMock> tournamentDelegateMock;
    std::shared_ptr<TournamentController> tournamentController;

    void SetUp() override {
        tournamentDelegateMock = std::make_shared<TournamentDelegateMock>();
        tournamentController = std::make_shared<TournamentController>(
            TournamentController(tournamentDelegateMock));
    }

    void TearDown() override {
        // teardown code comes here
    }
};

// GET /tournaments/{id}
TEST_F(TournamentControllerTest, GetTournamentById_ErrorFormat) {
    crow::response badRequest = tournamentController->getTournament("");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = tournamentController->getTournament("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TournamentControllerTest, GetTournamentById) {
    std::shared_ptr<domain::Tournament> expectedTournament =
        std::make_shared<domain::Tournament>();
    expectedTournament->Id() = "my-tournament-id";
    expectedTournament->Name() = "Tournament1";

    EXPECT_CALL(*tournamentDelegateMock,
                GetTournament(testing::Eq(std::string("my-tournament-id"))))
        .WillOnce(testing::Return(expectedTournament));

    crow::response response = tournamentController->getTournament("my-tournament-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTournament->Id(), jsonResponse["id"].s());
    EXPECT_EQ(expectedTournament->Name(), jsonResponse["name"].s());
}

TEST_F(TournamentControllerTest, GetTournamentNotFound) {
    EXPECT_CALL(*tournamentDelegateMock,
                GetTournament(testing::Eq(std::string("my-tournament-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = tournamentController->getTournament("my-tournament-id");
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// GET /tournaments
TEST_F(TournamentControllerTest, GetAllTournaments) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    auto tournament1 = std::make_shared<domain::Tournament>();
    tournament1->Id() = "tournament-1";
    tournament1->Name() = "Tournament1";

    auto tournament2 = std::make_shared<domain::Tournament>();
    tournament2->Id() = "tournament-2";
    tournament2->Name() = "Tournament2";

    tournaments.push_back(tournament1);
    tournaments.push_back(tournament2);

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    crow::response response = tournamentController->ReadAll();
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(2, jsonResponse.size());
}

TEST_F(TournamentControllerTest, GetAllTournaments_EmptyList) {
    std::vector<std::shared_ptr<domain::Tournament>> emptyTournaments;

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(emptyTournaments));

    crow::response response = tournamentController->ReadAll();
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(0, jsonResponse.size());
}

// POST /tournaments
TEST_F(TournamentControllerTest, CreateTournamentTest) {
    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTournament),
            testing::Return("new-tournament-id")));

    nlohmann::json tournamentRequestBody = {
            {"id", "new-id"},
        {"name", "Tournament1"}
    };

    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(),
              capturedTournament.Name());
}

TEST_F(TournamentControllerTest, CreateTournament_ManualIDRejected) {
    nlohmann::json tournamentRequestBody = {
        {"id", "manual-id"},
        {"name", "Tournament1"}
    };

    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("ID is not manually assignable", response.body);
}

TEST_F(TournamentControllerTest, CreateTournament_Conflict) {
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::Return("tournament-id-1"))
        .WillOnce(testing::Throw(DuplicateException("duplicate tournament")));

    nlohmann::json tournament1RequestBody = {{"name", "Tournament1"}};
    crow::request tournamentRequest1;
    tournamentRequest1.body = tournament1RequestBody.dump();

    nlohmann::json tournament2RequestBody = {{"name", "Tournament1"}};
    crow::request tournamentRequest2;
    tournamentRequest2.body = tournament2RequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest1);
    crow::response conflictResponse = tournamentController->CreateTournament(tournamentRequest2);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(crow::CONFLICT, conflictResponse.code);
}

TEST_F(TournamentControllerTest, CreateTournament_InvalidJSON) {
    crow::request tournamentRequest;
    tournamentRequest.body = "invalid json {{{";

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

// PATCH /tournaments/{id}
TEST_F(TournamentControllerTest, UpdateTournamentTest) {
    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_))
        .WillOnce(testing::DoAll(testing::SaveArg<0>(&capturedTournament), testing::Return("id1")));

    nlohmann::json tournamentRequestBody = {
        {"name", "Summer Update"}
    };

    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->updateTournament(
        tournamentRequest, "tournament-id");

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
    EXPECT_EQ("tournament-id", capturedTournament.Id());
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(),
              capturedTournament.Name());
}

TEST_F(TournamentControllerTest, UpdateTournament_NotFound) {
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_))
        .WillOnce(testing::Throw(NotFoundException("Tournament not found")));

    nlohmann::json tournamentRequestBody = {
        {"name", "Tournament1 Update"}
    };

    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->updateTournament(
        tournamentRequest, "non-existent-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TournamentControllerTest, UpdateTournament_InvalidIDFormat) {
    crow::request tournamentRequest;
    tournamentRequest.body = R"({"name": "Updated Tournament"})";

    crow::response response = tournamentController->updateTournament(
        tournamentRequest, "invalid#id*");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

TEST_F(TournamentControllerTest, UpdateTournament_InvalidJSON) {
    crow::request tournamentRequest;
    tournamentRequest.body = "invalid json {{{";

    crow::response response = tournamentController->updateTournament(
        tournamentRequest, "tournament-id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
}

TEST_F(TournamentControllerTest, UpdateTournament_IDMismatch) {
    nlohmann::json tournamentRequestBody = {
        {"id", "different-id"},
        {"name", "Tournament1"}
    };

    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->updateTournament(
        tournamentRequest, "tournament-id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("ID is not editable", response.body);
}
// TEST(TournamentControllerTest, CreateTournament) {
// std::shared_ptr<TournamentController> tournamentController;
// tournamentController->ReadAll();
// std::string id = "ID";
// std::string name = "Name";

// domain::Team team = {id,name};

// EXPECT_EQ(team.Id.c_str(), id);
// EXPECT_EQ(team.Name.c_str(), name);
// }
