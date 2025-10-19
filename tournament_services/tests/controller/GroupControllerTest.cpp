// GroupControllerTest.cpp
// Pruebas minimas requeridas para GroupController (sin acentos)

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "controller/GroupController.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using ::testing::StrictMock;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Truly;
using ::testing::_;
using ::testing::Eq;

class MockGroupDelegate : public IGroupDelegate {
public:
    MOCK_METHOD((std::expected<std::string,std::string>), CreateGroup,
                (const std::string_view& tournamentId, const domain::Group& group), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>), GetGroups,
                (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>), GetGroup,
                (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateGroup,
                (const std::string_view& tournamentId, const domain::Group& group), (override));
    MOCK_METHOD((std::expected<void, std::string>), RemoveGroup,
                (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateTeams,
                (const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams), (override));
};

struct GroupControllerFixture : ::testing::Test {
    std::shared_ptr<StrictMock<MockGroupDelegate>> delegateMock;
    std::shared_ptr<GroupController> controller;

    void SetUp() override {
        delegateMock = std::make_shared<StrictMock<MockGroupDelegate>>();
        controller   = std::make_shared<GroupController>(delegateMock);
    }
};

/* ============================================================================
 * 1-CREATE 201: transformar JSON -> Group, transferir a delegate, HTTP 201
 * ==========================================================================*/
TEST_F(GroupControllerFixture, CreateGroup_201_TransfersDomainValues) {
    nlohmann::json body = { {"id","g1"}, {"name","Group A"} };
    crow::request req; req.body = body.dump();

    domain::Group captured{};
    EXPECT_CALL(*delegateMock, CreateGroup(Eq(std::string_view{"t1"}), _))
        .WillOnce(DoAll(
            SaveArg<1>(&captured),
            Return(std::expected<std::string,std::string>{"g1"})
        ));

    auto res = controller->CreateGroup(req, "t1");
    EXPECT_EQ(res.code, crow::CREATED);
    EXPECT_EQ(res.get_header_value("location"), "g1");
    EXPECT_EQ(captured.Id(),   "g1");
    EXPECT_EQ(captured.Name(), "Group A");
}
// CHECK CHANGE group-conflict to group-already-exists
/* ============================================================================
 * 2-CREATE 409: transformar JSON -> Group, transferir, conflicto -> 409
 * ==========================================================================*/
TEST_F(GroupControllerFixture, CreateGroup_409_Conflict) {
    nlohmann::json body = { {"id","dup"}, {"name","Group X"} };
    crow::request req; req.body = body.dump();

    domain::Group captured{};
    EXPECT_CALL(*delegateMock, CreateGroup(Eq(std::string_view{"t1"}), _))
        .WillOnce(DoAll(
            SaveArg<1>(&captured),
            Return(std::unexpected(std::string{"group-already-exists"}))
        ));

    auto res = controller->CreateGroup(req, "t1");
    EXPECT_EQ(res.code, crow::CONFLICT);
    EXPECT_EQ(captured.Id(), "dup");
    EXPECT_EQ(captured.Name(), "Group X");
}

/* ============================================================================
 * 3-READ BY ID 200: validar ids, simular objeto y HTTP 200
 * ==========================================================================*/
TEST_F(GroupControllerFixture, ReadById_200_Found) {
    auto g = std::make_shared<domain::Group>();
    g->Id() = "g1"; g->Name() = "Group A";

    EXPECT_CALL(*delegateMock, GetGroup(Eq(std::string_view{"t1"}), Eq(std::string_view{"g1"})))
        .WillOnce(Return(std::expected<std::shared_ptr<domain::Group>,std::string>{g}));

    auto res = controller->GetGroup("t1", "g1");
    EXPECT_EQ(res.code, crow::OK);
    EXPECT_EQ(res.get_header_value("content-type"), "application/json");

    auto json = nlohmann::json::parse(res.body);
    EXPECT_EQ(json.at("id").get<std::string>(), "g1");
    EXPECT_EQ(json.at("name").get<std::string>(), "Group A");
}


// CHECK AND MODIFY CODE FROM GROUP CONTROLLER
/* ============================================================================
 * 4-READ BY ID 404: ids correctos, nulo -> 404
 * ==========================================================================*/
TEST_F(GroupControllerFixture, ReadById_404_NotFound) {
    EXPECT_CALL(*delegateMock, GetGroup(Eq(std::string_view{"t1"}), Eq(std::string_view{"missing"})))
        .WillOnce(Return(std::unexpected(std::string{"group-not-found"})));

    auto res = controller->GetGroup("t1", "missing");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}


//
/* ============================================================================
 * 5-UPDATE 204: transformar JSON -> Group, transferir y NO_CONTENT (PATCH)
 * ==========================================================================*/
TEST_F(GroupControllerFixture, UpdateGroup_204_NoContent) {
    nlohmann::json body = { {"id","DIFF"}, {"name","Updated"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*delegateMock,
        UpdateGroup(Eq(std::string_view{"t1"}),
            Truly([](const domain::Group& g){
                // id viene de la ruta, no del body
                return g.Id() == "g1" && g.Name() == "Updated" && g.TournamentId() == "t1";
            })))
        .WillOnce(Return(std::expected<void,std::string>{}));

    auto res = controller->UpdateGroup(req, "t1", "g1");
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}


//
/* ============================================================================
 * 6-UPDATE 404: transformar JSON -> Group, not-found -> 404
 * ==========================================================================*/
TEST_F(GroupControllerFixture, UpdateGroup_404_NotFound) {
    nlohmann::json body = { {"id","whatever"}, {"name","Name"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*delegateMock,
        UpdateGroup(Eq(std::string_view{"t1"}), _))
        .WillOnce(Return(std::unexpected(std::string{"group-not-found"})));

    auto res = controller->UpdateGroup(req, "t1", "g1");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}


//
/* ============================================================================
 * 7-ADD TEAM 204: transformar JSON -> Team[], transferir y NO_CONTENT
 * ==========================================================================*/
TEST_F(GroupControllerFixture, AddTeam_204_NoContent) {
    nlohmann::json body = nlohmann::json::array({ {{"id","p1"},{"name","P1"}} });
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*delegateMock,
        UpdateTeams(Eq(std::string_view{"t1"}), Eq(std::string_view{"g1"}),
                    Truly([](const std::vector<domain::Team>& v){
                        return v.size() == 1 && v[0].Id == "p1" && v[0].Name == "P1";
                    })))
        .WillOnce(Return(std::expected<void,std::string>{}));

    auto res = controller->UpdateTeams(req, "t1", "g1");
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

// CHECK DELETED ":"
/* ============================================================================
 * 8- ADD TEAM 422: equipo no existe
 * ==========================================================================*/
TEST_F(GroupControllerFixture, AddTeam_422_TeamNotFound) {
    nlohmann::json body = nlohmann::json::array({ {{"id","missing"},{"name","X"}} });
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*delegateMock, UpdateTeams(Eq(std::string_view{"t1"}), Eq(std::string_view{"g1"}), _))
        .WillOnce(Return(std::unexpected(std::string{"team-not-found"})));

    auto res = controller->UpdateTeams(req, "t1", "g1");
    EXPECT_EQ(res.code, 422);
}

/* ============================================================================
 * 9-ADD TEAM 422: grupo lleno
 * ==========================================================================*/
TEST_F(GroupControllerFixture, AddTeam_422_GroupFull) {
    nlohmann::json body = nlohmann::json::array({ {{"id","p9"},{"name","X"}} });
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*delegateMock, UpdateTeams(Eq(std::string_view{"t1"}), Eq(std::string_view{"g1"}), _))
        .WillOnce(Return(std::unexpected(std::string{"group-full"})));

    auto res = controller->UpdateTeams(req, "t1", "g1");
    EXPECT_EQ(res.code, 422);
}
