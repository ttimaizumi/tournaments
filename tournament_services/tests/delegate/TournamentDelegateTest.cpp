//
// Created by root on 10/9/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "domain/Tournament.hpp"
#include "persistence/repository/IRepository.hpp"
#include "delegate/TournamentDelegate.hpp"

class TournamentRepositoryMock : public IRepository<domain::Tournament, std::string_view> {
public:
    MOCK_METHOD(std::string_view, Create, (const domain::Tournament&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string_view), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Tournament&), (override));
    MOCK_METHOD(bool, Delete, (std::string_view), (override));
};

class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> repositoryMock;
    std::shared_ptr<TournamentDelegate> tournamentDelegate;

    void SetUp() override {
        repositoryMock = std::make_shared<TournamentRepositoryMock>();
        tournamentDelegate = std::make_shared<TournamentDelegate>(repositoryMock);
    }
};

// Test 1: Crear torneo - inserción válida
TEST_F(TournamentDelegateTest, SaveTournament_ValidTournament_ReturnsId) {
    domain::Tournament tournament("Mundial 2025");

    EXPECT_CALL(*repositoryMock, Create(testing::_))
        .WillOnce(testing::Return("generated-id"));

    auto result = tournamentDelegate->SaveTournament(tournament);

    EXPECT_EQ("generated-id", result);
}

// Test 2: Crear torneo - inserción fallida
TEST_F(TournamentDelegateTest, SaveTournament_DuplicateTournament_ThrowsException) {
    domain::Tournament tournament("Duplicate Tournament");

    EXPECT_CALL(*repositoryMock, Create(testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Tournament already exists")));

    EXPECT_THROW(tournamentDelegate->SaveTournament(tournament), std::runtime_error);
}

// Test 3: Buscar torneo por ID - resultado con objeto
TEST_F(TournamentDelegateTest, GetTournament_ValidId_ReturnsTournament) {
    auto expectedTournament = std::make_shared<domain::Tournament>("Mundial 2025");
    expectedTournament->Id() = "tournament-id";

    EXPECT_CALL(*repositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(expectedTournament));

    auto result = tournamentDelegate->GetTournament("tournament-id");

    ASSERT_NE(nullptr, result);
    EXPECT_EQ("tournament-id", result->Id());
    EXPECT_EQ("Mundial 2025", result->Name());
}

// Test 4: Buscar torneo por ID - resultado nulo
TEST_F(TournamentDelegateTest, GetTournament_InvalidId_ReturnsNull) {
    EXPECT_CALL(*repositoryMock, ReadById(testing::Eq("invalid-id")))
        .WillOnce(testing::Return(nullptr));

    auto result = tournamentDelegate->GetTournament("invalid-id");

    EXPECT_EQ(nullptr, result);
}

// Test 5: Buscar torneos - lista con objetos
TEST_F(TournamentDelegateTest, GetAllTournaments_WithTournaments_ReturnsList) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments = {
        std::make_shared<domain::Tournament>("Mundial 2025"),
        std::make_shared<domain::Tournament>("Copa America")
    };

    EXPECT_CALL(*repositoryMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    auto result = tournamentDelegate->GetAllTournaments();

    EXPECT_EQ(2, result.size());
    EXPECT_EQ("Mundial 2025", result[0]->Name());
    EXPECT_EQ("Copa America", result[1]->Name());
}

// Test 6: Buscar torneos - lista vacía
TEST_F(TournamentDelegateTest, GetAllTournaments_NoTournaments_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Tournament>> emptyList;

    EXPECT_CALL(*repositoryMock, ReadAll())
        .WillOnce(testing::Return(emptyList));

    auto result = tournamentDelegate->GetAllTournaments();

    EXPECT_EQ(0, result.size());
}

// Test 7: Actualizar torneo - actualización válida
TEST_F(TournamentDelegateTest, UpdateTournament_ValidTournament_ReturnsId) {
    domain::Tournament tournament("Updated Tournament");
    tournament.Id() = "tournament-id";

    EXPECT_CALL(*repositoryMock, Update(testing::_))
        .WillOnce(testing::Return("tournament-id"));

    auto result = tournamentDelegate->UpdateTournament(tournament);

    EXPECT_EQ("tournament-id", result);
}

// Test 8: Actualizar torneo - ID no encontrado
TEST_F(TournamentDelegateTest, UpdateTournament_InvalidId_ThrowsException) {
    domain::Tournament tournament("Tournament");
    tournament.Id() = "invalid-id";

    EXPECT_CALL(*repositoryMock, Update(testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Tournament not found")));

    EXPECT_THROW(tournamentDelegate->UpdateTournament(tournament), std::runtime_error);
}