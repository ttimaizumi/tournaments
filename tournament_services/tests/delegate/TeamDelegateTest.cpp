//
// Created by root on 10/8/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "domain/Team.hpp"
#include "persistence/repository/IRepository.hpp"
#include "delegate/TeamDelegate.hpp"

class TeamRepositoryMock : public IRepository<domain::Team, std::string_view> {
public:
    MOCK_METHOD(std::string_view, Create, (const domain::Team&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team&), (override));
    MOCK_METHOD(bool, Delete, (std::string_view), (override));
};

class TeamDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TeamRepositoryMock> repositoryMock;
    std::shared_ptr<TeamDelegate> teamDelegate;

    void SetUp() override {
        repositoryMock = std::make_shared<TeamRepositoryMock>();
        teamDelegate = std::make_shared<TeamDelegate>(repositoryMock);
    }
};

// Test 1: Crear equipo - inserción válida
TEST_F(TeamDelegateTest, SaveTeam_ValidTeam_ReturnsId) {
    domain::Team team{"", "Team A"};

    EXPECT_CALL(*repositoryMock, Create(testing::_))
        .WillOnce(testing::Return("generated-id"));

    auto result = teamDelegate->SaveTeam(team);

    EXPECT_EQ("generated-id", result);
}

// Test 2: Crear equipo - inserción fallida (ya existe)
// Nota: Según tu implementación actual, no manejas std::expected en TeamDelegate
// Este test asume que agregarás manejo de errores
TEST_F(TeamDelegateTest, SaveTeam_DuplicateTeam_ThrowsException) {
    domain::Team team{"", "Duplicate Team"};

    EXPECT_CALL(*repositoryMock, Create(testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Team already exists")));

    EXPECT_THROW(teamDelegate->SaveTeam(team), std::runtime_error);
}

// Test 3: Buscar equipo por ID - resultado con objeto
TEST_F(TeamDelegateTest, GetTeam_ValidId_ReturnsTeam) {
    auto expectedTeam = std::make_shared<domain::Team>(domain::Team{"team-id", "Team A"});

    EXPECT_CALL(*repositoryMock, ReadById(testing::Eq("team-id")))
        .WillOnce(testing::Return(expectedTeam));

    auto result = teamDelegate->GetTeam("team-id");

    ASSERT_NE(nullptr, result);
    EXPECT_EQ("team-id", result->Id);
    EXPECT_EQ("Team A", result->Name);
}

// Test 4: Buscar equipo por ID - resultado nulo
TEST_F(TeamDelegateTest, GetTeam_InvalidId_ReturnsNull) {
    EXPECT_CALL(*repositoryMock, ReadById(testing::Eq("invalid-id")))
        .WillOnce(testing::Return(nullptr));

    auto result = teamDelegate->GetTeam("invalid-id");

    EXPECT_EQ(nullptr, result);
}

// Test 5: Buscar equipos - lista con objetos
TEST_F(TeamDelegateTest, GetAllTeams_WithTeams_ReturnsList) {
    std::vector<std::shared_ptr<domain::Team>> teams = {
        std::make_shared<domain::Team>(domain::Team{"id-1", "Team 1"}),
        std::make_shared<domain::Team>(domain::Team{"id-2", "Team 2"})
    };

    EXPECT_CALL(*repositoryMock, ReadAll())
        .WillOnce(testing::Return(teams));

    auto result = teamDelegate->GetAllTeams();

    EXPECT_EQ(2, result.size());
    EXPECT_EQ("Team 1", result[0]->Name);
    EXPECT_EQ("Team 2", result[1]->Name);
}

// Test 6: Buscar equipos - lista vacía
TEST_F(TeamDelegateTest, GetAllTeams_NoTeams_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Team>> emptyList;

    EXPECT_CALL(*repositoryMock, ReadAll())
        .WillOnce(testing::Return(emptyList));

    auto result = teamDelegate->GetAllTeams();

    EXPECT_EQ(0, result.size());
}

// Test 7: Actualizar equipo - actualización válida
TEST_F(TeamDelegateTest, UpdateTeam_ValidTeam_ReturnsId) {
    domain::Team team{"team-id", "Updated Team"};

    EXPECT_CALL(*repositoryMock, Update(testing::_))
        .WillOnce(testing::Return("team-id"));

    auto result = teamDelegate->UpdateTeam(team);

    EXPECT_EQ("team-id", result);
}

// Test 8: Actualizar equipo - ID no encontrado
TEST_F(TeamDelegateTest, UpdateTeam_InvalidId_ThrowsException) {
    domain::Team team{"invalid-id", "Team Name"};

    EXPECT_CALL(*repositoryMock, Update(testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Team not found")));

    EXPECT_THROW(teamDelegate->UpdateTeam(team), std::runtime_error);
}