#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <expected>

#include "domain/Team.hpp"
#include "delegate/TeamDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"

// Mock del repositorio
class MockTeamRepository : public IRepository<domain::Team, std::string_view> {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view id), (override));
    MOCK_METHOD(std::string_view, Create, (const domain::Team& entity), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team& entity), (override));
    MOCK_METHOD(void, Delete, (std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
};

class TeamDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MockTeamRepository> mockRepository;
    std::shared_ptr<TeamDelegate> teamDelegate;

    void SetUp() override {
        mockRepository = std::make_shared<MockTeamRepository>();
        teamDelegate = std::make_shared<TeamDelegate>(mockRepository);
    }
};

// ========== Tests para CreateTeam ==========

// Validar creación exitosa con el valor transferido al repositorio
TEST_F(TeamDelegateTest, CreateTeam_ValidValue_ReturnsGeneratedId) {
    domain::Team newTeam{"", "New Team"};
    std::string_view expectedId = "generated-id";

    // Validar que el equipo transferido tenga el nombre correcto
    EXPECT_CALL(*mockRepository, Create(testing::Field(&domain::Team::Name, "New Team")))
        .WillOnce(testing::Return(expectedId));

    auto result = teamDelegate->CreateTeam(newTeam);

    EXPECT_EQ(result, expectedId);
}

// Validar creación fallida (si usas std::expected en TeamDelegate)
// Nota: Esto requeriría cambiar la firma de CreateTeam a retornar std::expected
TEST_F(TeamDelegateTest, CreateTeam_DuplicateName_ThrowsException) {
    domain::Team duplicateTeam{"", "Duplicate Team"};

    EXPECT_CALL(*mockRepository, Create(testing::Field(&domain::Team::Name, "Duplicate Team")))
        .WillOnce(testing::Throw(DuplicateException("A team with the same name already exists.")));

    EXPECT_THROW({
        teamDelegate->CreateTeam(duplicateTeam);
    }, DuplicateException);
}

// ========== Tests para GetTeam (búsqueda por ID) ==========

// Validar búsqueda exitosa con valor transferido y objeto retornado
TEST_F(TeamDelegateTest, GetTeam_ValidId_ReturnsTeamObject) {
    auto expectedTeam = std::make_shared<domain::Team>(domain::Team{"test-id", "Test Team"});
    
    // Validar que el ID transferido sea exactamente "test-id"
    EXPECT_CALL(*mockRepository, ReadById(testing::StrEq("test-id")))
        .WillOnce(testing::Return(expectedTeam));

    auto result = teamDelegate->GetTeam("test-id");

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Id, "test-id");
    EXPECT_EQ(result->Name, "Test Team");
}

// Validar búsqueda sin resultado (nullptr)
TEST_F(TeamDelegateTest, GetTeam_NonExistentId_ReturnsNullptr) {
    EXPECT_CALL(*mockRepository, ReadById(testing::StrEq("non-existent-id")))
        .WillOnce(testing::Return(nullptr));

    auto result = teamDelegate->GetTeam("non-existent-id");

    EXPECT_EQ(result, nullptr);
}

// ========== Tests para GetAllTeams ==========

// Simular lista con objetos
TEST_F(TeamDelegateTest, GetAllTeams_ReturnsMultipleTeams) {
    std::vector<std::shared_ptr<domain::Team>> teams = {
        std::make_shared<domain::Team>(domain::Team{"id1", "Team One"}),
        std::make_shared<domain::Team>(domain::Team{"id2", "Team Two"}),
        std::make_shared<domain::Team>(domain::Team{"id3", "Team Three"})
    };

    EXPECT_CALL(*mockRepository, ReadAll())
        .WillOnce(testing::Return(teams));

    auto result = teamDelegate->GetAllTeams();

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]->Id, "id1");
    EXPECT_EQ(result[0]->Name, "Team One");
    EXPECT_EQ(result[1]->Id, "id2");
    EXPECT_EQ(result[1]->Name, "Team Two");
    EXPECT_EQ(result[2]->Id, "id3");
    EXPECT_EQ(result[2]->Name, "Team Three");
}

// Simular lista vacía
TEST_F(TeamDelegateTest, GetAllTeams_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Team>> emptyTeams;

    EXPECT_CALL(*mockRepository, ReadAll())
        .WillOnce(testing::Return(emptyTeams));

    auto result = teamDelegate->GetAllTeams();

    EXPECT_TRUE(result.empty());
}

// ========== Tests para UpdateTeam ==========

// Validar actualización exitosa verificando el valor transferido
TEST_F(TeamDelegateTest, UpdateTeam_ValidTeam_ReturnsSuccessfully) {
    domain::Team updatedTeam{"existing-id", "Updated Team Name"};
    std::string_view expectedResult = "existing-id";

    // Validar que el equipo transferido tenga el ID y nombre correctos
    EXPECT_CALL(*mockRepository, Update(testing::AllOf(
        testing::Field(&domain::Team::Id, "existing-id"),
        testing::Field(&domain::Team::Name, "Updated Team Name")
    )))
        .WillOnce(testing::Return(expectedResult));

    auto result = teamDelegate->UpdateTeam(updatedTeam);

    EXPECT_EQ(result, expectedResult);
}

// Validar actualización fallida por equipo no encontrado
TEST_F(TeamDelegateTest, UpdateTeam_NonExistentTeam_ThrowsNotFoundException) {
    domain::Team nonExistentTeam{"non-existent-id", "Some Team"};

    EXPECT_CALL(*mockRepository, Update(testing::Field(&domain::Team::Id, "non-existent-id")))
        .WillOnce(testing::Throw(NotFoundException("Team not found for update.")));

    EXPECT_THROW({
        teamDelegate->UpdateTeam(nonExistentTeam);
    }, NotFoundException);
}

// Validar actualización con formato inválido
TEST_F(TeamDelegateTest, UpdateTeam_InvalidFormat_ThrowsException) {
    domain::Team invalidTeam{"invalid-format-id", "Team Name"};

    EXPECT_CALL(*mockRepository, Update(testing::Field(&domain::Team::Id, "invalid-format-id")))
        .WillOnce(testing::Throw(InvalidFormatException("Invalid ID format.")));

    EXPECT_THROW({
        teamDelegate->UpdateTeam(invalidTeam);
    }, InvalidFormatException);
}

// ========== Tests para DeleteTeam ==========

TEST_F(TeamDelegateTest, DeleteTeam_Success) {
    EXPECT_CALL(*mockRepository, Delete(testing::StrEq("team-to-delete")))
        .Times(1);

    EXPECT_NO_THROW({
        teamDelegate->DeleteTeam("team-to-delete");
    });
}

TEST_F(TeamDelegateTest, DeleteTeam_NotFound) {
    EXPECT_CALL(*mockRepository, Delete(testing::StrEq("non-existent-id")))
        .WillOnce(testing::Throw(NotFoundException("Team not found for deletion.")));

    EXPECT_THROW({
        teamDelegate->DeleteTeam("non-existent-id");
    }, NotFoundException);
}

TEST_F(TeamDelegateTest, DeleteTeam_InvalidFormat) {
    EXPECT_CALL(*mockRepository, Delete(testing::StrEq("invalid-id")))
        .WillOnce(testing::Throw(InvalidFormatException("Invalid ID format.")));

    EXPECT_THROW({
        teamDelegate->DeleteTeam("invalid-id");
    }, InvalidFormatException);
}  

