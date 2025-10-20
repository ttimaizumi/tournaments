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

// Test 2: Crear equipo - verifica que el objeto Team se transfiere correctamente
TEST_F(TeamDelegateTest, SaveTeam_TransfersCorrectTeamToRepository) {
    domain::Team capturedTeam;
    domain::Team inputTeam{"", "Team Alpha"};

    EXPECT_CALL(*repositoryMock, Create(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTeam),
                    testing::Return("new-id-456")
            ));

    auto result = teamDelegate->SaveTeam(inputTeam);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("Team Alpha", capturedTeam.Name);
    EXPECT_EQ("", capturedTeam.Id);
    EXPECT_EQ("new-id-456", result.value());
}

// Test 3: Buscar equipo por ID - resultado con objeto válido
TEST_F(TeamDelegateTest, GetTeam_ValidId_ReturnsTeamObject) {
    auto expectedTeam = std::make_shared<domain::Team>(
            domain::Team{"team-id-456", "Team Beta"}
    );

    EXPECT_CALL(*repositoryMock, ReadById(testing::Eq("team-id-456")))
            .WillOnce(testing::Return(expectedTeam));

    auto result = teamDelegate->GetTeam("team-id-456");

    ASSERT_NE(nullptr, result);
    EXPECT_EQ("team-id-456", result->Id);
    EXPECT_EQ("Team Beta", result->Name);
}

// Test 4: Buscar equipo por ID - resultado nulo cuando no existe
TEST_F(TeamDelegateTest, GetTeam_InvalidId_ReturnsNullptr) {
    EXPECT_CALL(*repositoryMock, ReadById(testing::Eq("non-existent-id")))
            .WillOnce(testing::Return(nullptr));

    auto result = teamDelegate->GetTeam("non-existent-id");

    EXPECT_EQ(nullptr, result);
}

// Test 5: Buscar equipo por ID - manejo de excepción retorna nullptr
TEST_F(TeamDelegateTest, GetTeam_RepositoryThrowsException_ReturnsNullptr) {
    EXPECT_CALL(*repositoryMock, ReadById(testing::Eq("error-id")))
            .WillOnce(testing::Throw(std::runtime_error("Database error")));

    auto result = teamDelegate->GetTeam("error-id");

    EXPECT_EQ(nullptr, result);
}

// Test 6: Buscar equipo por ID - verifica que se transfiere el ID correcto
TEST_F(TeamDelegateTest, GetTeam_TransfersCorrectIdToRepository) {
    std::string capturedId;
    auto mockTeam = std::make_shared<domain::Team>(domain::Team{"test-id", "Test"});

    EXPECT_CALL(*repositoryMock, ReadById(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedId),
                    testing::Return(mockTeam)
            ));

    teamDelegate->GetTeam("specific-id-789");

    EXPECT_EQ("specific-id-789", capturedId);
}

// Test 7: Buscar todos los equipos - lista con múltiples objetos
TEST_F(TeamDelegateTest, GetAllTeams_WithMultipleTeams_ReturnsList) {
    std::vector<std::shared_ptr<domain::Team>> teams = {
            std::make_shared<domain::Team>(domain::Team{"id-1", "Team 1"}),
            std::make_shared<domain::Team>(domain::Team{"id-2", "Team 2"}),
            std::make_shared<domain::Team>(domain::Team{"id-3", "Team 3"})
    };

    EXPECT_CALL(*repositoryMock, ReadAll())
            .WillOnce(testing::Return(teams));

    auto result = teamDelegate->GetAllTeams();

    EXPECT_EQ(3, result.size());
    EXPECT_EQ("id-1", result[0]->Id);
    EXPECT_EQ("Team 1", result[0]->Name);
    EXPECT_EQ("id-2", result[1]->Id);
    EXPECT_EQ("Team 2", result[1]->Name);
    EXPECT_EQ("id-3", result[2]->Id);
    EXPECT_EQ("Team 3", result[2]->Name);
}

// Test 8: Buscar todos los equipos - lista vacía
TEST_F(TeamDelegateTest, GetAllTeams_EmptyDatabase_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Team>> emptyList;

    EXPECT_CALL(*repositoryMock, ReadAll())
            .WillOnce(testing::Return(emptyList));

    auto result = teamDelegate->GetAllTeams();

    EXPECT_EQ(0, result.size());
    EXPECT_TRUE(result.empty());
}

// Test 9: Actualizar equipo - actualización exitosa retorna ID
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

// Test 10: Actualizar equipo - verifica transferencia correcta al repositorio
TEST_F(TeamDelegateTest, UpdateTeam_TransfersCorrectTeamToRepository) {
    domain::Team capturedTeam;
    domain::Team inputTeam{"update-id-789", "Modified Team"};

    EXPECT_CALL(*repositoryMock, Update(testing::_))
            .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTeam),
                    testing::Return("update-id-789")
            ));

    auto result = teamDelegate->UpdateTeam(inputTeam);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("update-id-789", capturedTeam.Id);
    EXPECT_EQ("Modified Team", capturedTeam.Name);
    EXPECT_EQ("update-id-789", result.value());
}

// Test 11: Actualizar equipo - verifica que Update es llamado
TEST_F(TeamDelegateTest, UpdateTeam_CallsRepositoryUpdate) {
    domain::Team team{"test-id", "Test Team"};

    EXPECT_CALL(*repositoryMock, Update(testing::_))
            .Times(1)
            .WillOnce(testing::Return("test-id"));

    auto result = teamDelegate->UpdateTeam(team);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("test-id", result.value());
}

// Test 12: Crear equipo - inserción fallida retorna error usando std::expected
TEST_F(TeamDelegateTest, SaveTeam_FailedInsertion_ReturnsExpectedError) {
    domain::Team team{"", "Duplicate Team"};

    EXPECT_CALL(*repositoryMock, Create(testing::_))
            .WillOnce(testing::Throw(std::runtime_error("Database error")));

    auto result = teamDelegate->SaveTeam(team);

    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("Failed to create team") != std::string::npos);
}

// Test 13: Actualizar equipo - ID no encontrado retorna error usando std::expected
TEST_F(TeamDelegateTest, UpdateTeam_TeamNotFound_ReturnsExpectedError) {
    domain::Team team{"non-existent-id", "Some Team"};

    EXPECT_CALL(*repositoryMock, Update(testing::_))
            .WillOnce(testing::Return(""));  // Empty string indicates not found

    auto result = teamDelegate->UpdateTeam(team);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Team not found", result.error());
}