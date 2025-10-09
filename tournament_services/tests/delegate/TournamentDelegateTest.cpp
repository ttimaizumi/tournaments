// TournamentDelegateTest.cpp
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
struct MockQueueMessageProducer : IQueueMessageProducer {
    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& queue), (override));
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

// REQ: crear torneo -> validar objeto enviado a Repository y que regresa el id generado.
// Ademas debe enviar mensaje a la cola cuando hay id.
TEST_F(TournamentDelegateFixture, CreateTournament_Success_ReturnsId_And_SendsMessage) {
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

// REQ: crear torneo -> insercion fallida usando senalizacion (aqui id vacio) y NO enviar mensaje.
TEST_F(TournamentDelegateFixture, CreateTournament_Failure_ReturnsEmpty_NoMessage) {
    auto in = std::make_shared<domain::Tournament>();
    in->Id() = "dup";
    in->Name() = "X";

    EXPECT_CALL(*repo, Create(_))
        .WillOnce(Return(std::string{}));

    EXPECT_CALL(*producer, SendMessage(_, _)).Times(0);

    auto id = delegate->CreateTournament(in);
    EXPECT_TRUE(id.empty());
}

// REQ: read-by-id -> validar id enviado a Repository, simular resultado objeto y validar contenido.
TEST_F(TournamentDelegateFixture, ReadById_Found_ReturnsObject) {
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

// REQ: read-by-id -> validar id enviado a Repository, simular resultado nulo y validar nullptr.
TEST_F(TournamentDelegateFixture, ReadById_NotFound_ReturnsNull) {
    EXPECT_CALL(*repo, ReadById(Eq(std::string{"zzz"})))
        .WillOnce(Return(nullptr));

    auto got = delegate->ReadById("zzz");
    EXPECT_EQ(got, nullptr);
}

// REQ: read-all -> simular lista con objetos y validar que se regresa igual.
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

// REQ: read-all -> simular lista vacia.
TEST_F(TournamentDelegateFixture, ReadAll_Empty) {
    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));

    auto result = delegate->ReadAll();
    EXPECT_TRUE(result.empty());
}

// REQ: update -> buscar por id, validar objeto enviado a Update y simular exito.
TEST_F(TournamentDelegateFixture, UpdateTournament_Success) {
    domain::Tournament in;
    in.Id() = "t1";
    in.Name() = "New Name";

    // primero debe existir
    EXPECT_CALL(*repo, ReadById(Eq(std::string{"t1"})))
        .WillOnce(Return(std::make_shared<domain::Tournament>()));

    // luego debe llamar Update con los valores esperados y devolver id no vacio
    EXPECT_CALL(*repo,
        Update(Truly([](const domain::Tournament& t){
            return t.Id() == "t1" && t.Name() == "New Name";
        }))
    ).WillOnce(Return(std::string{"t1"}));

    bool ok = delegate->UpdateTournament(in);
    EXPECT_TRUE(ok);
}

// REQ: update -> si no existe (busqueda no exitosa) debe regresar error (aqui false) y NO llamar Update.
TEST_F(TournamentDelegateFixture, UpdateTournament_NotFound) {
    domain::Tournament in;
    in.Id() = "missing";
    in.Name() = "X";

    EXPECT_CALL(*repo, ReadById(Eq(std::string{"missing"})))
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*repo, Update(_)).Times(0);

    bool ok = delegate->UpdateTournament(in);
    EXPECT_FALSE(ok);
}
