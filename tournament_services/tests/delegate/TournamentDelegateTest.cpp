// TournamentDelegateTest.cpp - requerimientos minimos

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "delegate/TournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "domain/Tournament.hpp"

using ::testing::StrictMock;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::Truly;
using ::testing::Eq;
using ::testing::_;

// Mock del repositorio de torneos segun la plantilla IRepository<domain::Tournament, std::string>
struct MockTournamentRepository : IRepository<domain::Tournament, std::string> {
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& t), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& t), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
};

// Mock del productor de mensajes via la interfaz (no la clase concreta)
class MockQueueMessageProducer : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage,
                (const std::string& message, const std::string& queue),
                (override));
};


class TournamentDelegateFixture : public ::testing::Test {
protected:
    std::shared_ptr<StrictMock<MockTournamentRepository>> repo;
    std::shared_ptr<StrictMock<MockQueueMessageProducer>> producer;
    std::unique_ptr<TournamentDelegate> delegate;

    void SetUp() override {
        repo     = std::make_shared<StrictMock<MockTournamentRepository>>();
        producer = std::make_shared<StrictMock<MockQueueMessageProducer>>();
        delegate = std::make_unique<TournamentDelegate>(repo, producer);
    }
};

/* =====================================================================================
 * REQUERIMIENTO: Creacion valida
 * "validar que el valor que se le transfiera a TournamentRepository es el esperado.
 *  Simular una insercion valida y que la respuesta sea el ID generado"
 * - Se valida objeto enviado a repo->Create(...)
 * - Repo devuelve "gen-1"
 * - Delegate devuelve "gen-1"
 * - Adicional (no requerido, pero realista): si hay id, se envia mensaje
 * ===================================================================================== */
TEST_F(TournamentDelegateFixture, CreateTournament_Success_TransfersToRepository_ReturnsId_And_SendsMessage) {
    auto in = std::make_shared<domain::Tournament>();
    in->Id() = "t1";
    in->Name() = "Alpha";

    EXPECT_CALL(*repo,
        Create(Truly([](const domain::Tournament& t){
            return t.Id() == "t1" && t.Name() == "Alpha";
        }))
    ).WillOnce(Return(std::string{"gen-1"}));

    EXPECT_CALL(*producer,
        SendMessage(Eq(std::string_view{"gen-1"}), Eq(std::string_view{"tournament.created"}))
    ).Times(1);

    auto id = delegate->CreateTournament(in);
    EXPECT_EQ(id, "gen-1");
}

/* =====================================================================================
 * REQUERIMIENTO: Creacion fallida usando std::expected
 * "validar que el valor que se le transfiera a TournamentRepository es el esperado.
 *  Simular una insercion fallida y que la respuesta sea un error o mensaje usando std::expected"
 * - Se valida objeto enviado a repo->Create(...)
 * - Repo devuelve string vacio -> conflicto
 * - Delegate::CreateTournamentEx(...) devuelve expected en error "conflict"
 * ===================================================================================== */
TEST_F(TournamentDelegateFixture, CreateTournament_Failure_TransfersToRepository_ReturnsExpectedError) {
    auto in = std::make_shared<domain::Tournament>();
    in->Id() = "dup";
    in->Name() = "X";

    domain::Tournament captured{};
    EXPECT_CALL(*repo, Create(_))
        .WillOnce(DoAll(
            ::testing::SaveArg<0>(&captured),
            Return(std::string{}) // simula conflicto
        ));

    auto r = delegate->CreateTournamentEx(in);

    // expected en error
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "conflict");

    // validar objeto transferido
    EXPECT_EQ(captured.Id(), "dup");
    EXPECT_EQ(captured.Name(), "X");

    // al fallar no se envia mensaje
    EXPECT_CALL(*producer, SendMessage(_, _)).Times(0);
}

/* =====================================================================================
 * REQUERIMIENTO: ReadById encontrado
 * "validar que el valor que se le transfiera a TournamentRepository es el esperado.
 *  Simular el resultado con un objeto y validar valores del objeto"
 * ===================================================================================== */
TEST_F(TournamentDelegateFixture, ReadById_Found_TransfersIdToRepository_ReturnsObject) {
    auto stored = std::make_shared<domain::Tournament>();
    stored->Id() = "t9";
    stored->Name() = "Gamma";

    EXPECT_CALL(*repo, ReadById(Eq(std::string{"t9"})))
        .WillOnce(Return(stored));

    auto got = delegate->ReadById("t9");
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->Id(), "t9");
    EXPECT_EQ(got->Name(), "Gamma");
}

/* =====================================================================================
 * REQUERIMIENTO: ReadById no encontrado
 * "validar que el valor que se le transfiera a TournamentRepository es el esperado.
 *  Simular el resultado nulo y validar nullptr"
 * ===================================================================================== */
TEST_F(TournamentDelegateFixture, ReadById_NotFound_TransfersIdToRepository_ReturnsNull) {
    EXPECT_CALL(*repo, ReadById(Eq(std::string{"zzz"})))
        .WillOnce(Return(nullptr));

    auto got = delegate->ReadById("zzz");
    EXPECT_EQ(got, nullptr);
}

/* =====================================================================================
 * REQUERIMIENTO: ReadAll con items
 * "Simular el resultado con una lista de objetos de TournamentRepository"
 * ===================================================================================== */
TEST_F(TournamentDelegateFixture, ReadAll_WithItems) {
    auto t1 = std::make_shared<domain::Tournament>();
    t1->Id() = "a"; t1->Name() = "A";

    auto t2 = std::make_shared<domain::Tournament>();
    t2->Id() = "b"; t2->Name() = "B";

    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{t1, t2}));

    auto result = delegate->ReadAll();
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0]->Id(), "a");
    EXPECT_EQ(result[0]->Name(), "A");
    EXPECT_EQ(result[1]->Id(), "b");
    EXPECT_EQ(result[1]->Name(), "B");
}

/* =====================================================================================
 * REQUERIMIENTO: ReadAll vacio
 * "Simular el resultado con una lista vacia de TournamentRepository"
 * ===================================================================================== */
TEST_F(TournamentDelegateFixture, ReadAll_Empty) {
    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));

    auto result = delegate->ReadAll();
    EXPECT_TRUE(result.empty());
}

/* =====================================================================================
 * REQUERIMIENTO: Update exitoso
 * "validar la busqueda de TournamentRepository por ID, validar el valor transferido a Update.
 *  Simular resultado exitoso"
 * - Primero ReadById("t1") devuelve existente
 * - Se valida objeto pasado a Update(Id y Name)
 * - Repo::Update devuelve "t1" (exito) -> delegate::UpdateTournament(...) true
 * ===================================================================================== */
TEST_F(TournamentDelegateFixture, UpdateTournament_Success_ReadsById_TransfersToUpdate) {
    domain::Tournament in;
    in.Id() = "t1";
    in.Name() = "New Name";

    EXPECT_CALL(*repo, ReadById(Eq(std::string{"t1"})))
        .WillOnce(Return(std::make_shared<domain::Tournament>()));

    EXPECT_CALL(*repo,
        Update(Truly([](const domain::Tournament& t){
            return t.Id() == "t1" && t.Name() == "New Name";
        }))
    ).WillOnce(Return(std::string{"t1"}));

    bool ok = delegate->UpdateTournament(in);
    EXPECT_TRUE(ok);
}

/* =====================================================================================
 * REQUERIMIENTO: Update no encontrado usando std::expected
 * "validar la busqueda de TournamentRepository por ID. Simular resultado de busqueda no
 *  exitoso y regresar error o mensaje usando std::expected"
 * - ReadById("missing") -> nullptr
 * - UpdateTournamentEx(...) devuelve expected en error "not found"
 * ===================================================================================== */
TEST_F(TournamentDelegateFixture, UpdateTournament_NotFound_ReadsById_ReturnsExpectedError) {
    EXPECT_CALL(*repo, ReadById(Eq(std::string{"missing"})))
        .WillOnce(Return(nullptr));

    auto r = delegate->UpdateTournamentEx(domain::Tournament{}, std::string{"missing"}, std::string{"X"});
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "not found");
}
