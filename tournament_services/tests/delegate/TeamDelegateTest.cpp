//
// Created by root on 10/8/25.
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "domain/Team.hpp"
#include "persistence/repository/IRepository.hpp"
#include "delegate/TeamDelegate.hpp"

namespace { // anonymous namespace to avoid symbol collisions

class TeamRepositoryMock : public IRepository<domain::Team, std::string_view> {
public:
    MOCK_METHOD(std::string_view, Create, (const domain::Team&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team&), (override));
    MOCK_METHOD(void, Delete, (std::string_view), (override));
};

class TeamDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TeamRepositoryMock> repositoryMock;
    std::shared_ptr<TeamDelegate> teamDelegate;

    void SetUp() override {
        repositoryMock = std::make_shared<TeamRepositoryMock>();
        teamDelegate = std::make_shared<TeamDelegate>(repositoryMock);
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(repositoryMock.get());
    }
};

// Test 1: Crear equipo - inserción válida, verifica transferencia y retorna ID
TEST_F(TeamDelegateTest, SaveTeam_ValidTeam_ReturnsGeneratedId) {
    domain::Team team{"", "New Team"};
    const std::string expectedId = "generated-id-123";
    domain::Team capturedTeam;

    EXPECT_CALL(*repositoryMock, Create(testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTeam),
            testing::Return(expectedId)
        ));

    auto result = teamDelegate->SaveTeam(team);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(expectedId, result.value());
    EXPECT_EQ("New Team", capturedTeam.Name);
}

// Test 2: Crear equipo - inserción fallida retorna std::unexpected
TEST_F(TeamDelegateTest, SaveTeam_FailedInsertion_ReturnsError) {
    domain::Team team{"", "Failing Team"};

    EXPECT_CALL(*repositoryMock, Create(testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Database error")));

    auto result = teamDelegate->SaveTeam(team);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Error creating team"));
}

// Test 3: Buscar equipo por ID - validar transferencia y retornar objeto
TEST_F(TeamDelegateTest, GetTeam_ValidId_ReturnsTeamObject) {
    auto expectedTeam = std::make_shared<domain::Team>(
        domain::Team{"team-id-456", "Team Beta"}
    );
    std::string_view capturedId;

    EXPECT_CALL(*repositoryMock, ReadById(testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedId),
            testing::Return(expectedTeam)
        ));

    auto result = teamDelegate->GetTeam("team-id-456");

    ASSERT_TRUE(result.has_value());
    auto team = result.value();
    ASSERT_NE(nullptr, team);
    EXPECT_EQ("team-id-456", team->Id);
    EXPECT_EQ("Team Beta", team->Name);
    EXPECT_EQ("team-id-456", capturedId);
}

// Test 4: Buscar equipo por ID - resultado nulo cuando no existe
TEST_F(TeamDelegateTest, GetTeam_InvalidId_ReturnsNullptr) {
    std::string_view capturedId;

    EXPECT_CALL(*repositoryMock, ReadById(testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedId),
            testing::Return(nullptr)
        ));

    auto result = teamDelegate->GetTeam("non-existent-id");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(nullptr, result.value());
    EXPECT_EQ("non-existent-id", capturedId);
}

// Test 5: Buscar todos los equipos - resultado con lista de objetos
TEST_F(TeamDelegateTest, GetAllTeams_WithMultipleTeams_ReturnsList) {
    std::vector<std::shared_ptr<domain::Team>> teams = {
        std::make_shared<domain::Team>(domain::Team{"id-1", "Team 1"}),
        std::make_shared<domain::Team>(domain::Team{"id-2", "Team 2"}),
        std::make_shared<domain::Team>(domain::Team{"id-3", "Team 3"})
    };

    EXPECT_CALL(*repositoryMock, ReadAll())
        .WillOnce(testing::Return(teams));

    auto result = teamDelegate->GetAllTeams();

    ASSERT_TRUE(result.has_value());
    auto teamList = result.value();
    EXPECT_EQ(3, teamList.size());
    EXPECT_EQ("id-1", teamList[0]->Id);
    EXPECT_EQ("Team 1", teamList[0]->Name);
    EXPECT_EQ("id-2", teamList[1]->Id);
    EXPECT_EQ("Team 2", teamList[1]->Name);
    EXPECT_EQ("id-3", teamList[2]->Id);
    EXPECT_EQ("Team 3", teamList[2]->Name);
}

// Test 6: Buscar todos los equipos - resultado con lista vacía
TEST_F(TeamDelegateTest, GetAllTeams_EmptyDatabase_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Team>> emptyList;

    EXPECT_CALL(*repositoryMock, ReadAll())
        .WillOnce(testing::Return(emptyList));

    auto result = teamDelegate->GetAllTeams();

    ASSERT_TRUE(result.has_value());
    auto teamList = result.value();
    EXPECT_EQ(0, teamList.size());
    EXPECT_TRUE(teamList.empty());
}

// Test 7: Actualizar equipo - validar búsqueda y transferencia, resultado exitoso
TEST_F(TeamDelegateTest, UpdateTeam_ValidTeam_ReturnsTeamId) {
    domain::Team team{"team-id-update", "Updated Team Name"};
    domain::Team capturedTeam;

    EXPECT_CALL(*repositoryMock, Update(testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTeam),
            testing::Return("team-id-update")
        ));

    auto result = teamDelegate->UpdateTeam(team);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("team-id-update", result.value());
    EXPECT_EQ("team-id-update", capturedTeam.Id);
    EXPECT_EQ("Updated Team Name", capturedTeam.Name);
}

// Test 8: Actualizar equipo - ID no encontrado retorna std::unexpected
TEST_F(TeamDelegateTest, UpdateTeam_NonExistentId_ReturnsError) {
    domain::Team team{"non-existent-id", "Some Team"};

    EXPECT_CALL(*repositoryMock, Update(testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Team not found")));

    auto result = teamDelegate->UpdateTeam(team);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Error updating team"));
}

} // end anonymous namespace
