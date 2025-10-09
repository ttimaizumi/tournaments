// TournamentControllerTest.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "controller/TournamentController.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "domain/Tournament.hpp"

using ::testing::StrictMock;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SaveArg;
using ::testing::Truly;
using ::testing::_;
using ::testing::Eq;

// mock del delegate con todos los metodos que usa el controller
class MockTournamentDelegate : public ITournamentDelegate {
public:
    MOCK_METHOD(std::string, CreateTournament, (std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string_view), (override));
    MOCK_METHOD(bool, UpdateTournament, (const domain::Tournament&), (override));
};

struct TournamentControllerFixture : ::testing::Test {
    std::shared_ptr<StrictMock<MockTournamentDelegate>> delegateMock;
    std::shared_ptr<TournamentController> controller;

    void SetUp() override {
        delegateMock = std::make_shared<StrictMock<MockTournamentDelegate>>();
        controller   = std::make_shared<TournamentController>(delegateMock);
    }
};

// crear torneo -> transforma JSON a dominio, pasa al delegate,
// delegate regresa id no vacio -> HTTP 201 con header Location
TEST_F(TournamentControllerFixture, CreateTournament_201_LocationHeader_And_TransfersDomainValues) {
    nlohmann::json reqBody = { {"id","t1"}, {"name","Summer Cup"} };
    crow::request req; req.body = reqBody.dump();

    std::shared_ptr<domain::Tournament> captured;
    EXPECT_CALL(*delegateMock, CreateTournament(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(std::string{"t1"})));

    crow::response res = controller->CreateTournament(req);

    EXPECT_EQ(res.code, crow::CREATED);
    ASSERT_NE(captured, nullptr);
    EXPECT_EQ(captured->Id(),   "t1");
    EXPECT_EQ(captured->Name(), "Summer Cup");
}

// crear torneo -> JSON invalido -> 400
TEST_F(TournamentControllerFixture, CreateTournament_400_BadJson) {
    crow::request req; req.body = "{ not-json ";
    auto res = controller->CreateTournament(req);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// crear torneo -> insercion falla (id vacio) -> 409
TEST_F(TournamentControllerFixture, CreateTournament_409_Conflict) {
    nlohmann::json body = { {"id","dup"}, {"name","X"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*delegateMock, CreateTournament(_))
        .WillOnce(Return(std::string{})); // simulamos fallo

    auto res = controller->CreateTournament(req);
    EXPECT_EQ(res.code, crow::CONFLICT);
}

// read-all -> lista con elementos -> 200 con arreglo json
TEST_F(TournamentControllerFixture, ReadAll_200_WithItems) {
    // construir objetos de dominio sin pasar json al ctor
    auto t1 = std::make_shared<domain::Tournament>();
    t1->Id() = "t1";
    t1->Name() = "Alpha";

    auto t2 = std::make_shared<domain::Tournament>();
    t2->Id() = "t2";
    t2->Name() = "Beta";

    EXPECT_CALL(*delegateMock, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{t1, t2}));

    crow::response res = controller->ReadAll();
    EXPECT_EQ(res.code, crow::OK);

    auto json = nlohmann::json::parse(res.body);
    ASSERT_TRUE(json.is_array());
    ASSERT_EQ(json.size(), 2);
    EXPECT_EQ(json[0].at("id").get<std::string>(), "t1");
    EXPECT_EQ(json[0].at("name").get<std::string>(), "Alpha");
    EXPECT_EQ(json[1].at("id").get<std::string>(), "t2");
    EXPECT_EQ(json[1].at("name").get<std::string>(), "Beta");
}

// read-all -> lista vacia -> 200 con arreglo vacio
TEST_F(TournamentControllerFixture, ReadAll_200_EmptyList) {
    EXPECT_CALL(*delegateMock, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));

    crow::response res = controller->ReadAll();
    EXPECT_EQ(res.code, crow::OK);

    auto json = nlohmann::json::parse(res.body);
    ASSERT_TRUE(json.is_array());
    EXPECT_TRUE(json.empty());
}

// read-by-id -> id con formato invalido -> 400
TEST_F(TournamentControllerFixture, ReadById_400_BadIdFormat) {
    auto res = controller->ReadById("bad id!!"); // no pasa el regex
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// read-by-id -> encontrado -> 200 con objeto json
TEST_F(TournamentControllerFixture, ReadById_200_Found) {
    // evitar pasar json al ctor (no hay ctor json); armamos el objeto manualmente
    auto t = std::make_shared<domain::Tournament>();
    t->Id() = "t9";
    t->Name() = "Gamma";

    EXPECT_CALL(*delegateMock, ReadById(Eq(std::string_view{"t9"})))
        .WillOnce(Return(t));

    auto res = controller->ReadById("t9");
    EXPECT_EQ(res.code, crow::OK);

    auto json = nlohmann::json::parse(res.body);
    EXPECT_EQ(json.at("id").get<std::string>(), "t9");
    EXPECT_EQ(json.at("name").get<std::string>(), "Gamma");
}

// read-by-id -> no encontrado -> 404
TEST_F(TournamentControllerFixture, ReadById_404_NotFound) {
    EXPECT_CALL(*delegateMock, ReadById(Eq(std::string_view{"zzz"})))
        .WillOnce(Return(nullptr));

    auto res = controller->ReadById("zzz");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// update -> transforma JSON a dominio y pasa al delegate
// delegate devuelve true -> 204
TEST_F(TournamentControllerFixture, UpdateTournament_204_NoContent_TransfersDomainValues) {
    nlohmann::json body = { {"id","t1"}, {"name","New Name"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*delegateMock,
        UpdateTournament(Truly([](const domain::Tournament& tt){
            return tt.Id() == "t1" && tt.Name() == "New Name";
        })))
        .WillOnce(Return(true));

    auto res = controller->UpdateTournament(req);
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

// update -> id no encontrado (delegate false) -> 404
TEST_F(TournamentControllerFixture, UpdateTournament_404_NotFound) {
    nlohmann::json body = { {"id","missing"}, {"name","X"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*delegateMock, UpdateTournament(_))
        .WillOnce(Return(false));

    auto res = controller->UpdateTournament(req);
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// update -> json invalido -> 400

TEST_F(TournamentControllerFixture, UpdateTournament_400_BadJson) {
    crow::request req; req.body = "not json";
    auto res = controller->UpdateTournament(req);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}
