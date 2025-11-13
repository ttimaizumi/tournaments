#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <expected>
#include <string_view>
#include "delegate/GroupDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using ::testing::StrictMock;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SaveArg;
using ::testing::Truly;
using ::testing::Eq;
using ::testing::_;

// Tournament repo mock (IRepository<domain::Tournament, std::string>)
struct MockTournamentRepo : IRepository<domain::Tournament, std::string> {
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament&), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament&), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
};

// Group repo mock
struct MockGroupRepo : IGroupRepository {
    MOCK_METHOD(std::shared_ptr<domain::Group>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, ReadAll, (), (override));
    MOCK_METHOD(std::string, Create, (const domain::Group& g), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group& g), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));

    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId,
                (const std::string_view& tournamentId), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId,
                (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId,
                (const std::string_view& tournamentId, const std::string_view& teamId), (override));
    MOCK_METHOD(void, UpdateGroupAddTeam,
                (const std::string_view& groupId, const std::shared_ptr<domain::Team>& team), (override));
};

// Team repo mock (IRepository<domain::Team, std::string>)
struct MockTeamRepo : IRepository<domain::Team, std::string_view> {
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::string_view, Create, (const domain::Team&), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team&), (override));
    MOCK_METHOD(void, Delete, (std::string_view id), (override));
};

class MockProducer : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage,
                (const std::string& message, const std::string& queue),
                (override));
};

class GroupDelegateFixture : public ::testing::Test {
protected:
    std::shared_ptr<StrictMock<MockTournamentRepo>> tRepo;
    std::shared_ptr<StrictMock<MockGroupRepo>>      gRepo;
    std::shared_ptr<StrictMock<MockTeamRepo>>       teamRepo;
    std::shared_ptr<StrictMock<MockProducer>>       producer;
    std::unique_ptr<GroupDelegate>                  delegate;

    void SetUp() override {
        tRepo    = std::make_shared<StrictMock<MockTournamentRepo>>();
        gRepo    = std::make_shared<StrictMock<MockGroupRepo>>();
        teamRepo = std::make_shared<StrictMock<MockTeamRepo>>();
        producer = std::make_shared<StrictMock<MockProducer>>();
        delegate = std::make_unique<GroupDelegate>(tRepo, gRepo, teamRepo, producer);
    }
};

TEST_F(GroupDelegateFixture, CreateGroup_Success_ReadsTournament_Transfers_Publishes_ReturnsId) {
    auto tour = std::make_shared<domain::Tournament>(); tour->Id() = "t1";
    EXPECT_CALL(*tRepo, ReadById(Eq(std::string{"t1"}))).WillOnce(Return(tour));

    domain::Group captured{};
    EXPECT_CALL(*gRepo, Create(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(std::string{"g1"})));

    EXPECT_CALL(*producer, SendMessage(Eq(std::string_view{"g1"}), Eq(std::string_view{"group.created"})))
        .Times(1);

    domain::Group g; g.Id() = "g1"; g.Name() = "A";
    auto r = delegate->CreateGroup("t1", g);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value(), "g1");
    EXPECT_EQ(captured.Id(), "g1");
    EXPECT_EQ(captured.TournamentId(), "t1");
}

TEST_F(GroupDelegateFixture, CreateGroup_Conflict_ExpectedError) {
    auto tour = std::make_shared<domain::Tournament>(); tour->Id() = "t1";
    EXPECT_CALL(*tRepo, ReadById(Eq(std::string{"t1"}))).WillOnce(Return(tour));
    EXPECT_CALL(*gRepo, Create(_)).WillOnce(Return(std::string{}));
    EXPECT_CALL(*producer, SendMessage(_, _)).Times(0);

    domain::Group g; g.Id() = "dup";
    auto r = delegate->CreateGroup("t1", g);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "group-already-exists");
}

TEST_F(GroupDelegateFixture, GetGroup_Found) {
    auto g = std::make_shared<domain::Group>(); g->Id() = "g1";
    EXPECT_CALL(*gRepo, FindByTournamentIdAndGroupId(Eq(std::string_view{"t1"}), Eq(std::string_view{"g1"})))
        .WillOnce(Return(g));

    auto r = delegate->GetGroup("t1", "g1");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value()->Id(), "g1");
}

TEST_F(GroupDelegateFixture, GetGroup_NotFound) {
    EXPECT_CALL(*gRepo, FindByTournamentIdAndGroupId(Eq(std::string_view{"t1"}), Eq(std::string_view{"missing"})))
        .WillOnce(Return(nullptr));

    auto r = delegate->GetGroup("t1", "missing");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "group-not-found");
}

TEST_F(GroupDelegateFixture, GetGroups_WithItems) {
    auto g1 = std::make_shared<domain::Group>(); g1->Id() = "a";
    auto g2 = std::make_shared<domain::Group>(); g2->Id() = "b";

    EXPECT_CALL(*gRepo, FindByTournamentId(Eq(std::string_view{"t1"})))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Group>>{g1, g2}));

    auto r = delegate->GetGroups("t1");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r.value().size(), 2u);
    EXPECT_EQ(r.value()[0]->Id(), "a");
    EXPECT_EQ(r.value()[1]->Id(), "b");
}

TEST_F(GroupDelegateFixture, GetGroups_Empty) {
    EXPECT_CALL(*gRepo, FindByTournamentId(Eq(std::string_view{"t1"})))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Group>>{}));

    auto r = delegate->GetGroups("t1");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().empty());
}

TEST_F(GroupDelegateFixture, UpdateGroup_Success) {
    EXPECT_CALL(*gRepo, FindByTournamentIdAndGroupId(Eq(std::string_view{"t1"}), Eq(std::string_view{"g1"})))
        .WillOnce(Return(std::make_shared<domain::Group>(domain::Group{})));

    domain::Group captured{};
    EXPECT_CALL(*gRepo,
        Update(Truly([&](const domain::Group& gg){
            return gg.Id() == "g1" && gg.Name() == "Updated";
        })))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(std::string{"g1"})));

    domain::Group in; in.Id() = "g1"; in.Name() = "Updated";
    auto r = delegate->UpdateGroup("t1", in);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(captured.Id(), "g1");
    EXPECT_EQ(captured.Name(), "Updated");
}

TEST_F(GroupDelegateFixture, UpdateGroup_NotFound_ExpectedError) {
    EXPECT_CALL(*gRepo, FindByTournamentIdAndGroupId(Eq(std::string_view{"t1"}), Eq(std::string_view{"g404"})))
        .WillOnce(Return(nullptr));

    domain::Group in; in.Id() = "g404";
    auto r = delegate->UpdateGroup("t1", in);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "group-not-found");
}

TEST_F(GroupDelegateFixture, UpdateTeams_AddsAndPublishesPerTeam) {
    auto g = std::make_shared<domain::Group>();
    g->Id() = "g1";

    EXPECT_CALL(*gRepo,
        FindByTournamentIdAndGroupId(
            Eq(std::string_view{"t1"}),
            Eq(std::string_view{"g1"})))
        .WillRepeatedly(Return(g));   // <- antes era WillOnce

    EXPECT_CALL(*gRepo,
        FindByTournamentIdAndTeamId(
            Eq(std::string_view{"t1"}),
            Eq(std::string_view{"p1"})))
        .WillOnce(Return(nullptr));

    auto persisted = std::make_shared<domain::Team>(domain::Team{"p1","P1"});

    EXPECT_CALL(*teamRepo, ReadById(Eq(std::string{"p1"})))
        .Times(2) // pre-check + anadir
        .WillRepeatedly(Return(persisted));

    EXPECT_CALL(*gRepo,
        UpdateGroupAddTeam(Eq(std::string_view{"g1"}), persisted))
        .Times(1);

    EXPECT_CALL(*producer,
        SendMessage(_, Eq(std::string_view{"group.team-added"})))
        .Times(1);

    std::vector<domain::Team> input{ domain::Team{"p1","P1"} };
    auto r = delegate->UpdateTeams("t1", "g1", input);
    EXPECT_TRUE(r.has_value());
}


TEST_F(GroupDelegateFixture, UpdateTeams_TeamNotFound_ExpectedError) {
    auto g = std::make_shared<domain::Group>(); g->Id() = "g1";
    EXPECT_CALL(*gRepo, FindByTournamentIdAndGroupId(
     Eq(std::string_view{"t1"}), Eq(std::string_view{"g1"})))
     .WillOnce(Return(g));


    EXPECT_CALL(*gRepo, FindByTournamentIdAndTeamId(Eq(std::string_view{"t1"}), Eq(std::string_view{"missing"})))
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*teamRepo, ReadById(Eq(std::string{"missing"})))
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*producer, SendMessage(_, _)).Times(0);
    EXPECT_CALL(*gRepo, UpdateGroupAddTeam(_, _)).Times(0);

    std::vector<domain::Team> input{ domain::Team{"missing","X"} };
    auto r = delegate->UpdateTeams("t1", "g1", input);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "team-not-found");
}
//
TEST_F(GroupDelegateFixture, UpdateTeams_GroupFull_ExpectedError) {
    auto g = std::make_shared<domain::Group>();
    g->Id() = "g1";
    // simulamos grupo totalmente lleno
    g->Teams().resize(32);

    EXPECT_CALL(*gRepo,
        FindByTournamentIdAndGroupId(
            Eq(std::string_view{"t1"}),
            Eq(std::string_view{"g1"})))
        .WillOnce(Return(g));

    // Si ya esta lleno, NO debe intentar buscar ni agregar equipos
    EXPECT_CALL(*gRepo, FindByTournamentIdAndTeamId(_, _)).Times(0);
    EXPECT_CALL(*teamRepo, ReadById(_)).Times(0);
    EXPECT_CALL(*gRepo, UpdateGroupAddTeam(_, _)).Times(0);
    EXPECT_CALL(*producer, SendMessage(_, _)).Times(0);

    std::vector<domain::Team> input{ domain::Team{"p9","P9"} };

    auto r = delegate->UpdateTeams("t1", "g1", input);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "group-full");
}
