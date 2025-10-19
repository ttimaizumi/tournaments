// teamdelegatetest.cpp — Pruebas mínimas esperadas para TeamDelegate

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <string_view>

#include "delegate/TeamDelegate.hpp"
#include "domain/Team.hpp"
#include "persistence/repository/IRepository.hpp"

using ::testing::StrictMock;
using ::testing::Return;
using ::testing::_;
using ::testing::DoAll;
using ::testing::SaveArg;

/* -----------------------------------------------------------------------------
 * Mock del repositorio que usa el TeamDelegate.
 * Se simulan sus métodos para aislar el comportamiento del delegate.
 * ---------------------------------------------------------------------------*/
struct MockTeamRepository : IRepository<domain::Team, std::string_view> {
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view id), (override));
    MOCK_METHOD(std::string_view, Create, (const domain::Team& team), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team& team), (override));
    MOCK_METHOD(void, Delete, (std::string_view id), (override));
};

/* -----------------------------------------------------------------------------
 * Fixture del delegate para reutilizar setUp en cada test.
 * Crea el StrictMock y el TeamDelegate bajo prueba.
 * ---------------------------------------------------------------------------*/
class TeamDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<StrictMock<MockTeamRepository>> repo;
    std::unique_ptr<TeamDelegate> delegate;

    void SetUp() override {
        repo = std::make_shared<StrictMock<MockTeamRepository>>();
        delegate = std::make_unique<TeamDelegate>(repo);
    }
};

/* =======================================================================================
 * 1) CREATE: validar que el valor transferido a TeamRepository es el esperado.
 *    Simular inserción válida y que la respuesta sea el ID generado.
 *    - Se captura el Team que el delegate manda a repo->Create(...)
 *    - Se valida que ID/Name coinciden con lo esperado
 *    - El repo responde "gen-1" y el delegate debe propagar ese ID
 * ======================================================================================= */
TEST_F(TeamDelegateTest, SaveTeam_Success_TransfersToRepository_ReturnsId) {
    domain::Team t{"gen-1", "Alpha"};

    domain::Team captured{};
    EXPECT_CALL(*repo, Create(_))
        .WillOnce(DoAll(
            SaveArg<0>(&captured),
            Return(std::string_view{"gen-1"})
        ));

    auto idView = delegate->SaveTeam(t);

    // Se propagó el ID generado por el repositorio
    EXPECT_FALSE(idView.empty());
    EXPECT_EQ(idView, std::string_view{"gen-1"});

    // Se transfirieron correctamente los valores al repositorio
    EXPECT_EQ(captured.Id,   "gen-1");
    EXPECT_EQ(captured.Name, "Alpha");
}

/* =======================================================================================
 * 2) CREATE: validar que el valor transferido a TeamRepository es el esperado.
 *    Simular inserción fallida y que la respuesta sea un error (std::expected).
 *    - Se captura el Team que el delegate manda al repo
 *    - El repo devuelve string_view vacío → conflicto
 *    - delegate->SaveTeamEx(...) debe regresar expected en estado de error "conflict"
 * ======================================================================================= */
TEST_F(TeamDelegateTest, SaveTeam_Failure_TransfersToRepository_ReturnsExpectedError) {
    domain::Team t{"dup-1", "Duplicate"};

    domain::Team captured{};
    EXPECT_CALL(*repo, Create(_))
        .WillOnce(DoAll(
            SaveArg<0>(&captured),
            Return(std::string_view{}) // simula conflicto
        ));

    auto r = delegate->SaveTeamEx(t);

    // expected en estado de error
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "conflict");

    // Se transfirieron correctamente los valores al repositorio
    EXPECT_EQ(captured.Id,   "dup-1");
    EXPECT_EQ(captured.Name, "Duplicate");
}

/* =======================================================================================
 * 3) READ BY ID: validar que el valor (ID) transferido a TeamRepository es el esperado.
 *    Simular resultado con un objeto y validar el objeto.
 * ======================================================================================= */
TEST_F(TeamDelegateTest, GetTeam_TransfersIdToRepository_Found_ReturnsObject) {
    auto teamPtr = std::make_shared<domain::Team>(domain::Team{"t1", "Lions"});

    EXPECT_CALL(*repo, ReadById(::testing::Eq(std::string_view{"t1"})))
        .WillOnce(Return(teamPtr));

    auto got = delegate->GetTeam("t1");
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->Id,   "t1");
    EXPECT_EQ(got->Name, "Lions");
}

/* =======================================================================================
 * 4) READ BY ID: validar que el valor (ID) transferido a TeamRepository es el esperado.
 *    Simular resultado nulo y validar nullptr.
 * ======================================================================================= */
TEST_F(TeamDelegateTest, GetTeam_TransfersIdToRepository_NotFound_ReturnsNullptr) {
    EXPECT_CALL(*repo, ReadById(::testing::Eq(std::string_view{"missing"})))
        .WillOnce(Return(nullptr));

    auto got = delegate->GetTeam("missing");
    EXPECT_EQ(got, nullptr);
}

/* =======================================================================================
 * 5) READ ALL: simular resultado con una lista de objetos del TeamRepository.
 * ======================================================================================= */
TEST_F(TeamDelegateTest, GetAllTeams_WithItems) {
    std::vector<std::shared_ptr<domain::Team>> items;
    items.push_back(std::make_shared<domain::Team>(domain::Team{"a", "A"}));
    items.push_back(std::make_shared<domain::Team>(domain::Team{"b", "B"}));

    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(Return(items));

    auto result = delegate->GetAllTeams();
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0]->Id,   "a");
    EXPECT_EQ(result[0]->Name, "A");
    EXPECT_EQ(result[1]->Id,   "b");
    EXPECT_EQ(result[1]->Name, "B");
}

/* =======================================================================================
 * 6) READ ALL: simular resultado con una lista vacía del TeamRepository.
 * ======================================================================================= */
TEST_F(TeamDelegateTest, GetAllTeams_Empty) {
    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

    auto result = delegate->GetAllTeams();
    EXPECT_TRUE(result.empty());
}

/* =======================================================================================
 * 7) UPDATE: validar búsqueda por ID en TeamRepository y el valor transferido a Update.
 *    Simular resultado exitoso.
 *    - Primero ReadById("u1") devuelve existente
 *    - Se captura el Team pasado a Update y se validan sus valores (Id/Name)
 *    - Update devuelve "u1" → delegate::UpdateTeam(...) debe regresar true
 * ======================================================================================= */
TEST_F(TeamDelegateTest, UpdateTeam_Success_ReadsById_TransfersToUpdate_ReturnsTrue) {
    domain::Team input{"u1", "Bravo"};
    auto existing = std::make_shared<domain::Team>(domain::Team{"u1", "OldName"});

    EXPECT_CALL(*repo, ReadById(::testing::Eq(std::string_view{"u1"})))
        .WillOnce(Return(existing));

    domain::Team captured{};
    EXPECT_CALL(*repo, Update(::testing::_))
        .WillOnce(DoAll(
            SaveArg<0>(&captured),
            Return(std::string_view{"u1"}) // ok
        ));

    bool ok = delegate->UpdateTeam(input);
    EXPECT_TRUE(ok);
    EXPECT_EQ(captured.Id,   "u1");
    EXPECT_EQ(captured.Name, "Bravo");
}

/* =======================================================================================
 * 8) UPDATE: validar búsqueda por ID en TeamRepository.
 *    Simular resultado de búsqueda no exitoso y regresar error (std::expected).
 *    - ReadById("missing") → nullptr
 *    - delegate::UpdateTeamEx(...) debe regresar expected en error "not found"
 * ======================================================================================= */
// Test From Ivanovich Push
TEST_F(TeamDelegateTest, UpdateTeam_NotFound_ReadsById_ReturnsExpectedError) {
    EXPECT_CALL(*repo, ReadById(::testing::Eq(std::string_view{"missing"})))
        .WillOnce(Return(nullptr)); // no existe

    auto r = delegate->UpdateTeamEx(domain::Team{"missing", "X"});
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "not found");
}
