#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "controller/GroupController.hpp"

class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateGroup, (const std::string_view& tournamentId, const domain::Group& group), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>), GetGroups, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>), GetGroup, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateGroup, (const std::string_view& tournamentId, const domain::Group& group), (override));
    MOCK_METHOD((std::expected<void, std::string>), RemoveGroup, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateTeams, (const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams), (override));
};

class GroupControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<GroupDelegateMock> groupDelegateMock;
    std::shared_ptr<GroupController> groupController;

    void SetUp() override {
        groupDelegateMock = std::make_shared<GroupDelegateMock>();
        groupController = std::make_shared<GroupController>(GroupController(groupDelegateMock));
    }
};

// Test 1: Crear grupo - transformación JSON a Group, transferir a delegate, HTTP 201
TEST_F(GroupControllerTest, CreateGroup_Success_Returns201) {
    domain::Group capturedGroup;

    EXPECT_CALL(*groupDelegateMock, CreateGroup(testing::Eq("tournament-1"), testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<1>(&capturedGroup),
            testing::Return(std::expected<std::string, std::string>("group-id-123"))
        ));

    nlohmann::json body = {{"name", "Group A"}, {"teams", nlohmann::json::array()}};
    crow::request req; req.body = body.dump();

    auto res = groupController->CreateGroup(req, "tournament-1");

    EXPECT_EQ(crow::CREATED, res.code);
    EXPECT_EQ("Group A", capturedGroup.Name());
    EXPECT_EQ("group-id-123", res.get_header_value("location"));
}

// Test 2: Crear grupo - simular error de inserción, HTTP 409
TEST_F(GroupControllerTest, CreateGroup_Error_Returns409) {
    EXPECT_CALL(*groupDelegateMock, CreateGroup(testing::Eq("tournament-1"), testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group already exists")));

    nlohmann::json body = {{"name", "Group A"}, {"teams", nlohmann::json::array()}};
    crow::request req; req.body = body.dump();

    auto res = groupController->CreateGroup(req, "tournament-1");

    EXPECT_EQ(409, res.code);
}

// Test 3: Buscar grupo por ID y torneo por ID - transferir a delegate, HTTP 200
TEST_F(GroupControllerTest, GetGroup_Success_Returns200) {
    auto expectedGroup = std::make_shared<domain::Group>("Group A", "group-1");
    expectedGroup->SetTournamentId("tournament-1");

    EXPECT_CALL(*groupDelegateMock, GetGroup(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(expectedGroup)));

    auto res = groupController->GetGroup("tournament-1", "group-1");
    auto json = crow::json::load(res.body);

    ASSERT_TRUE(json);
    EXPECT_EQ(crow::OK, res.code);
    EXPECT_EQ("Group A", json["name"].s());
    EXPECT_EQ("group-1", json["id"].s());
}

// Test 4: Buscar grupo por ID y torneo por ID - resultado nulo, HTTP 404
TEST_F(GroupControllerTest, GetGroup_NotFound_Returns404) {
    EXPECT_CALL(*groupDelegateMock, GetGroup(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group not found")));

    auto res = groupController->GetGroup("tournament-1", "group-1");

    EXPECT_EQ(crow::NOT_FOUND, res.code);
}

// Test 5: Actualizar grupo - transformación JSON a Group, transferir a delegate, HTTP 204
TEST_F(GroupControllerTest, UpdateGroup_Success_Returns204) {
    domain::Group captured;

    EXPECT_CALL(*groupDelegateMock, UpdateGroup(testing::Eq("tournament-1"), testing::_))
        .WillOnce(testing::DoAll(testing::SaveArg<1>(&captured), testing::Return(std::expected<void, std::string>())));

    nlohmann::json body = {{"name", "Updated Group A"}};
    crow::request req; req.body = body.dump();

    auto res = groupController->UpdateGroup(req, "tournament-1", "group-1");

    EXPECT_EQ(crow::NO_CONTENT, res.code);
    EXPECT_EQ("Updated Group A", captured.Name());
    EXPECT_EQ("group-1", captured.Id());
}

// Test 6: Actualizar grupo - simular ID no encontrado, HTTP 404
TEST_F(GroupControllerTest, UpdateGroup_NotFound_Returns404) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(testing::Eq("tournament-1"), testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group doesn't exist")));

    nlohmann::json body = {{"name", "Updated Group A"}};
    crow::request req; req.body = body.dump();

    auto res = groupController->UpdateGroup(req, "tournament-1", "group-1");

    EXPECT_EQ(crow::NOT_FOUND, res.code);
}

// Test 7: Agregar equipo a grupo - transformación JSON a Team, transferir a delegate, HTTP 204
TEST_F(GroupControllerTest, UpdateTeams_Success_Returns204) {
    std::vector<domain::Team> capturedTeams;

    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-1"), testing::Eq("group-1"), testing::_))
        .WillOnce(testing::DoAll(testing::SaveArg<2>(&capturedTeams), testing::Return(std::expected<void, std::string>())));

    nlohmann::json arr = nlohmann::json::array({ {{"id", "t1"}, {"name", "A"}}, {{"id", "t2"}, {"name", "B"}} });
    crow::request req; req.body = arr.dump();

    auto res = groupController->UpdateTeams(req, "tournament-1", "group-1");

    EXPECT_EQ(crow::NO_CONTENT, res.code);
    EXPECT_EQ(2, capturedTeams.size());
    EXPECT_EQ("t1", capturedTeams[0].Id);
    EXPECT_EQ("A", capturedTeams[0].Name);
}

// Test 8: Agregar equipo a grupo - simular equipo no existe, HTTP 422
TEST_F(GroupControllerTest, UpdateTeams_TeamNotFound_Returns422) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-1"), testing::Eq("group-1"), testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team not found")));

    nlohmann::json body = {{"id", "t1"}, {"name", "A"}};
    crow::request req; req.body = body.dump();

    auto res = groupController->UpdateTeams(req, "tournament-1", "group-1");

    EXPECT_EQ(422, res.code);
}

// Test 9: Agregar equipo a grupo - simular grupo lleno, HTTP 422
TEST_F(GroupControllerTest, UpdateTeams_GroupFull_Returns422) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-1"), testing::Eq("group-1"), testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group at max capacity")));

    nlohmann::json body = {{"id", "t1"}, {"name", "A"}};
    crow::request req; req.body = body.dump();

    auto res = groupController->UpdateTeams(req, "tournament-1", "group-1");

    EXPECT_EQ(422, res.code);
}
