#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/IRepository.hpp"
#include "delegate/GroupDelegate.hpp"

class TournamentRepositoryMock : public IRepository<domain::Tournament, std::string> {
public:
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Tournament>>), ReadAll, (), (override));
};

class GroupRepositoryMock : public IGroupRepository {
public:
    MOCK_METHOD(std::shared_ptr<domain::Group>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Group& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Group>>), ReadAll, (), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Group>>), FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId, (const std::string_view& tournamentId, const std::string_view& teamId), (override));
    MOCK_METHOD(void, UpdateGroupAddTeam, (const std::string_view& groupId, const std::shared_ptr<domain::Team>& team), (override));
};

class TeamRepositoryMock : public IRepository<domain::Team, std::string_view> {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view id), (override));
    MOCK_METHOD(std::string_view, Create, (const domain::Team& entity), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team& entity), (override));
    MOCK_METHOD(void, Delete, (std::string_view id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Team>>), ReadAll, (), (override));
};

class QueueMessageProducerMock : public QueueMessageProducer {
public:
    QueueMessageProducerMock() : QueueMessageProducer(nullptr) {}

    MOCK_METHOD(void, SendMessage, (const std::string_view&, const std::string_view&), (override));
};

class GroupDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<QueueMessageProducerMock> producerMockPtr;
    QueueMessageProducerMock* producerMock;
    std::shared_ptr<GroupRepositoryMock> groupRepositoryMock;
    std::shared_ptr<TeamRepositoryMock> teamRepositoryMock;
    std::shared_ptr<GroupDelegate> groupDelegate;

    void SetUp() override {
        producerMockPtr = std::make_shared<QueueMessageProducerMock>();
        producerMock = producerMockPtr.get();
        tournamentRepositoryMock = std::make_shared<TournamentRepositoryMock>();
        groupRepositoryMock = std::make_shared<GroupRepositoryMock>();
        teamRepositoryMock = std::make_shared<TeamRepositoryMock>();
        groupDelegate = std::make_shared<GroupDelegate>(
            tournamentRepositoryMock,
            groupRepositoryMock,
            teamRepositoryMock,
            producerMockPtr
        );
    }

    void TearDown() override {
    }
};

// Test CreateGroup - success with ID
TEST_F(GroupDelegateTest, CreateGroup_Success) {
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tournament-1";

    domain::Group capturedGroup;

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, Create(testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedGroup),
            testing::Return("group-id-123")
        ));

    domain::Group group("Group A");
    auto result = groupDelegate->CreateGroup("tournament-1", group);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("group-id-123", result.value());
    EXPECT_EQ("Group A", capturedGroup.Name());
    EXPECT_EQ("tournament-1", capturedGroup.TournamentId());
}

// Test CreateGroup - tournament doesn't exist
TEST_F(GroupDelegateTest, CreateGroup_TournamentNotFound) {
    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(nullptr));

    domain::Group group("Group A");
    auto result = groupDelegate->CreateGroup("tournament-1", group);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ("Tournament doesn't exist", result.error());
}

// Test CreateGroup - team doesn't exist
TEST_F(GroupDelegateTest, CreateGroup_TeamNotFound) {
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tournament-1";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*teamRepositoryMock, ReadById(testing::Eq("team-1")))
        .WillOnce(testing::Return(nullptr));

    domain::Group group("Group A");
    group.Teams().push_back(domain::Team{"team-1", "Team A"});

    auto result = groupDelegate->CreateGroup("tournament-1", group);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ("Team doesn't exist", result.error());
}

// Test GetGroup - success
TEST_F(GroupDelegateTest, GetGroup_Success) {
    auto expectedGroup = std::make_shared<domain::Group>("Group A", "group-1");
    expectedGroup->TournamentId() = "tournament-1";

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(expectedGroup));

    auto result = groupDelegate->GetGroup("tournament-1", "group-1");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("Group A", result.value()->Name());
    EXPECT_EQ("group-1", result.value()->Id());
}

// Test GetGroup - not found
TEST_F(GroupDelegateTest, GetGroup_NotFound) {
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(nullptr));

    auto result = groupDelegate->GetGroup("tournament-1", "group-1");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(nullptr, result.value());
}

// Test GetGroups - success with list
TEST_F(GroupDelegateTest, GetGroups_Success) {
    std::vector<std::shared_ptr<domain::Group>> groups;
    auto group1 = std::make_shared<domain::Group>("Group A", "group-1");
    auto group2 = std::make_shared<domain::Group>("Group B", "group-2");
    groups.push_back(group1);
    groups.push_back(group2);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(groups));

    auto result = groupDelegate->GetGroups("tournament-1");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(2, result.value().size());
}

// Test GetGroups - empty list
TEST_F(GroupDelegateTest, GetGroups_EmptyList) {
    std::vector<std::shared_ptr<domain::Group>> emptyGroups;

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(emptyGroups));

    auto result = groupDelegate->GetGroups("tournament-1");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(0, result.value().size());
}

// Test UpdateGroup - success
TEST_F(GroupDelegateTest, UpdateGroup_Success) {
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tournament-1";

    auto existingGroup = std::make_shared<domain::Group>("Old Group Name", "group-1");
    existingGroup->TournamentId() = "tournament-1";

    domain::Group capturedGroup;

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(existingGroup));

    EXPECT_CALL(*groupRepositoryMock, Update(testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedGroup),
            testing::Return("group-1")
        ));

    domain::Group group("Updated Group Name");
    group.Id() = "group-1";

    auto result = groupDelegate->UpdateGroup("tournament-1", group);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("Updated Group Name", capturedGroup.Name());
    EXPECT_EQ("tournament-1", capturedGroup.TournamentId());
}

// Test UpdateGroup - tournament not found
TEST_F(GroupDelegateTest, UpdateGroup_TournamentNotFound) {
    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(nullptr));

    domain::Group group("Updated Group Name");
    group.Id() = "group-1";

    auto result = groupDelegate->UpdateGroup("tournament-1", group);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ("Tournament doesn't exist", result.error());
}

// Test UpdateGroup - group not found
TEST_F(GroupDelegateTest, UpdateGroup_GroupNotFound) {
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tournament-1";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(nullptr));

    domain::Group group("Updated Group Name");
    group.Id() = "group-1";

    auto result = groupDelegate->UpdateGroup("tournament-1", group);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ("Group doesn't exist", result.error());
}

// Test UpdateTeams - success
TEST_F(GroupDelegateTest, UpdateTeams_Success) {
    auto group = std::make_shared<domain::Group>("Group A", "group-1");
    group->TournamentId() = "tournament-1";

    auto team1 = std::make_shared<domain::Team>(domain::Team{"team-1", "Team A"});
    auto team2 = std::make_shared<domain::Team>(domain::Team{"team-2", "Team B"});

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(group));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(testing::Eq("tournament-1"), testing::Eq("team-1")))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(testing::Eq("tournament-1"), testing::Eq("team-2")))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*teamRepositoryMock, ReadById(testing::Eq("team-1")))
        .WillOnce(testing::Return(team1));

    EXPECT_CALL(*teamRepositoryMock, ReadById(testing::Eq("team-2")))
        .WillOnce(testing::Return(team2));

    EXPECT_CALL(*groupRepositoryMock, UpdateGroupAddTeam(testing::Eq("group-1"), testing::_))
        .Times(2);

    std::vector<domain::Team> teams = {
        domain::Team{"team-1", "Team A"},
        domain::Team{"team-2", "Team B"}
    };

    auto result = groupDelegate->UpdateTeams("tournament-1", "group-1", teams);

    EXPECT_TRUE(result.has_value());
}

// Test UpdateTeams - group doesn't exist
TEST_F(GroupDelegateTest, UpdateTeams_GroupNotFound) {
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(nullptr));

    std::vector<domain::Team> teams = {domain::Team{"team-1", "Team A"}};

    auto result = groupDelegate->UpdateTeams("tournament-1", "group-1", teams);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ("Group doesn't exist", result.error());
}

// Test UpdateTeams - team doesn't exist
TEST_F(GroupDelegateTest, UpdateTeams_TeamNotFound) {
    auto group = std::make_shared<domain::Group>("Group A", "group-1");
    group->TournamentId() = "tournament-1";

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(group));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(testing::Eq("tournament-1"), testing::Eq("team-1")))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*teamRepositoryMock, ReadById(testing::Eq("team-1")))
        .WillOnce(testing::Return(nullptr));

    std::vector<domain::Team> teams = {domain::Team{"team-1", "Team A"}};

    auto result = groupDelegate->UpdateTeams("tournament-1", "group-1", teams);

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("team-1") != std::string::npos);
    EXPECT_TRUE(result.error().find("doesn't exist") != std::string::npos);
}

// Test UpdateTeams - group is full
TEST_F(GroupDelegateTest, UpdateTeams_GroupFull) {
    auto group = std::make_shared<domain::Group>("Group A", "group-1");
    group->TournamentId() = "tournament-1";
    // Add 15 teams to make it almost full
    for (int i = 0; i < 15; i++) {
        group->Teams().push_back(domain::Team{"team-" + std::to_string(i), "Team " + std::to_string(i)});
    }

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(group));

    std::vector<domain::Team> teams = {domain::Team{"team-99", "Team 99"}};

    auto result = groupDelegate->UpdateTeams("tournament-1", "group-1", teams);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ("Group at max capacity", result.error());
}

// Test UpdateTeams - team already exists in tournament
TEST_F(GroupDelegateTest, UpdateTeams_TeamAlreadyExists) {
    auto group = std::make_shared<domain::Group>("Group A", "group-1");
    group->TournamentId() = "tournament-1";

    auto existingGroupWithTeam = std::make_shared<domain::Group>("Group B", "group-2");

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(group));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(testing::Eq("tournament-1"), testing::Eq("team-1")))
        .WillOnce(testing::Return(existingGroupWithTeam));

    std::vector<domain::Team> teams = {domain::Team{"team-1", "Team A"}};

    auto result = groupDelegate->UpdateTeams("tournament-1", "group-1", teams);

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("team-1") != std::string::npos);
    EXPECT_TRUE(result.error().find("already exist") != std::string::npos);
}

// Test RemoveGroup - success
TEST_F(GroupDelegateTest, RemoveGroup_Success) {
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tournament-1";

    auto existingGroup = std::make_shared<domain::Group>("Group A", "group-1");
    existingGroup->TournamentId() = "tournament-1";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(existingGroup));

    EXPECT_CALL(*groupRepositoryMock, Delete(testing::Eq("group-1")))
        .Times(1);

    auto result = groupDelegate->RemoveGroup("tournament-1", "group-1");

    EXPECT_TRUE(result.has_value());
}

// Test RemoveGroup - tournament not found
TEST_F(GroupDelegateTest, RemoveGroup_TournamentNotFound) {
    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(nullptr));

    auto result = groupDelegate->RemoveGroup("tournament-1", "group-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ("Tournament doesn't exist", result.error());
}

// Test RemoveGroup - group not found
TEST_F(GroupDelegateTest, RemoveGroup_GroupNotFound) {
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tournament-1";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-1")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-1"), testing::Eq("group-1")))
        .WillOnce(testing::Return(nullptr));

    auto result = groupDelegate->RemoveGroup("tournament-1", "group-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ("Group doesn't exist", result.error());
}
