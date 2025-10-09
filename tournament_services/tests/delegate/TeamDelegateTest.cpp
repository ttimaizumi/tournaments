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

struct MockTeamRepository : IRepository<domain::Team, std::string_view> {
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view id), (override));
    MOCK_METHOD(std::string_view, Create, (const domain::Team& team), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team& team), (override));
    MOCK_METHOD(void, Delete, (std::string_view id), (override));
};

class TeamDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<StrictMock<MockTeamRepository>> repo;
    std::unique_ptr<TeamDelegate> delegate;

    void SetUp() override {
        repo = std::make_shared<StrictMock<MockTeamRepository>>();
        delegate = std::make_unique<TeamDelegate>(repo);
    }
};

/*** creacion valida -> usa SaveTeam (interfaz existente) ***/
TEST_F(TeamDelegateTest, SaveTeam_Success_ReturnsId) {
    domain::Team t{"gen-1", "Alpha"};

    EXPECT_CALL(*repo, Create(_))
        .WillOnce(Return(std::string_view{"gen-1"}));

    auto idView = delegate->SaveTeam(t);
    EXPECT_FALSE(idView.empty());
    EXPECT_EQ(idView, std::string_view{"gen-1"});
}

/*** creacion fallida -> variante con expected ***/
TEST_F(TeamDelegateTest, SaveTeam_Failure_ExpectedError) {
    domain::Team t{"dup-1", "Duplicate"};

    EXPECT_CALL(*repo, Create(_))
        .WillOnce(Return(std::string_view{})); // simula conflicto

    auto r = delegate->SaveTeamEx(t);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "conflict");
}

/*** buscar por id -> encontrado ***/
TEST_F(TeamDelegateTest, GetTeam_Found) {
    auto teamPtr = std::make_shared<domain::Team>(domain::Team{"t1", "Lions"});

    EXPECT_CALL(*repo, ReadById(::testing::Eq(std::string_view{"t1"})))
        .WillOnce(Return(teamPtr));

    auto got = delegate->GetTeam("t1");
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->Id, "t1");
    EXPECT_EQ(got->Name, "Lions");
}

/*** buscar por id -> no encontrado ***/
TEST_F(TeamDelegateTest, GetTeam_NotFound) {
    EXPECT_CALL(*repo, ReadById(::testing::Eq(std::string_view{"missing"})))
        .WillOnce(Return(nullptr));

    auto got = delegate->GetTeam("missing");
    EXPECT_EQ(got, nullptr);
}

/*** listar -> con elementos ***/
TEST_F(TeamDelegateTest, GetAllTeams_WithItems) {
    std::vector<std::shared_ptr<domain::Team>> items;
    items.push_back(std::make_shared<domain::Team>(domain::Team{"a", "A"}));
    items.push_back(std::make_shared<domain::Team>(domain::Team{"b", "B"}));

    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(Return(items));

    auto result = delegate->GetAllTeams();
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0]->Id, "a");
    EXPECT_EQ(result[0]->Name, "A");
    EXPECT_EQ(result[1]->Id, "b");
    EXPECT_EQ(result[1]->Name, "B");
}

/*** listar -> vacio ***/
TEST_F(TeamDelegateTest, GetAllTeams_Empty) {
    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

    auto result = delegate->GetAllTeams();
    EXPECT_TRUE(result.empty());
}

/*** actualizacion exitosa -> usa bool UpdateTeam (interfaz existente) ***/
TEST_F(TeamDelegateTest, UpdateTeam_Success_ReturnsTrue) {
    domain::Team input{"u1", "Bravo"};
    auto existing = std::make_shared<domain::Team>(domain::Team{"u1", "OldName"});

    EXPECT_CALL(*repo, ReadById(::testing::Eq(std::string_view{"u1"})))
        .WillOnce(Return(existing));

    domain::Team captured{};
    EXPECT_CALL(*repo, Update(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&captured),
            Return(std::string_view{"u1"}) // ok
        ));

    bool ok = delegate->UpdateTeam(input);
    EXPECT_TRUE(ok);
    EXPECT_EQ(captured.Id, "u1");
    EXPECT_EQ(captured.Name, "Bravo");
}

/*** actualizacion no exitosa -> variante con expected ***/
// Test From Ivanovich Push
TEST_F(TeamDelegateTest, UpdateTeam_NotFound_ExpectedError) {
    EXPECT_CALL(*repo, ReadById(::testing::Eq(std::string_view{"missing"})))
        .WillOnce(Return(nullptr)); // no existe

    auto r = delegate->UpdateTeamEx(domain::Team{"missing", "X"});
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "not found");
}
