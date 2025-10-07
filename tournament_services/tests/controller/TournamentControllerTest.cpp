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