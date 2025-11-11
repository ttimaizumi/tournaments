#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/IRepository.hpp"
#include "delegate/GroupDelegate.hpp"

namespace {
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
    };

    // Test 1: CreateGroup - simular lectura de Tournament, validar transferencia a GroupRepository, validar evento generado, validar ID
    TEST_F(GroupDelegateTest, CreateGroup_Success_ReturnsGroupId) {
        auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
        tournament->Id() = "tournament-1";

        domain::Group capturedGroup;

        EXPECT_CALL(*tournamentRepositoryMock, ReadById("tournament-1"))
            .WillOnce(testing::Return(tournament));

        EXPECT_CALL(*groupRepositoryMock, Create(testing::_))
            .WillOnce(testing::DoAll(testing::SaveArg<0>(&capturedGroup), testing::Return("group-id-123")));

        domain::Group group("Group A");
        auto result = groupDelegate->CreateGroup("tournament-1", group);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ("group-id-123", result.value());
        EXPECT_EQ("Group A", capturedGroup.Name());
        EXPECT_EQ("tournament-1", capturedGroup.TournamentId());
    }

    // Test 2: CreateGroup - simular error cuando grupo ya existe (std::unexpected)
    TEST_F(GroupDelegateTest, CreateGroup_GroupAlreadyExists_ReturnsError) {
        auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
        tournament->Id() = "tournament-1";

        EXPECT_CALL(*tournamentRepositoryMock, ReadById("tournament-1"))
            .WillOnce(testing::Return(tournament));

        EXPECT_CALL(*groupRepositoryMock, Create(testing::_))
            .WillOnce(testing::Throw(std::runtime_error("Group already exists")));

        domain::Group group("Group A");
        auto result = groupDelegate->CreateGroup("tournament-1", group);

        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error(), testing::HasSubstr("Group already exists"));
    }

    // Test 3: CreateGroup - simular error cuando se alcanza el máximo de equipos (std::unexpected)
    TEST_F(GroupDelegateTest, CreateGroup_MaxTeamsReached_ReturnsError) {
        auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
        tournament->Id() = "tournament-1";
        tournament->Format().MaxTeamsPerGroup() = 2;

        EXPECT_CALL(*tournamentRepositoryMock, ReadById("tournament-1"))
            .WillOnce(testing::Return(tournament));

        domain::Group group("Group A");
        group.MutableTeams().push_back(domain::Team{"team-1", "Team 1"});
        group.MutableTeams().push_back(domain::Team{"team-2", "Team 2"});
        group.MutableTeams().push_back(domain::Team{"team-3", "Team 3"}); // Exceeds max

        auto result = groupDelegate->CreateGroup("tournament-1", group);

        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error(), testing::HasSubstr("exceeds maximum teams"));
    }

    // Test 4: GetGroup - validar transferencia a GroupRepository, simular objeto resultado
    TEST_F(GroupDelegateTest, GetGroup_Success_ReturnsGroup) {
        auto expectedGroup = std::make_shared<domain::Group>("Group A", "group-1");
        expectedGroup->SetTournamentId("tournament-1");

        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
            .WillOnce(testing::Return(expectedGroup));

        auto result = groupDelegate->GetGroup("tournament-1", "group-1");

        ASSERT_TRUE(result.has_value());
        auto group = result.value();
        ASSERT_NE(nullptr, group);
        EXPECT_EQ("Group A", group->Name());
        EXPECT_EQ("group-1", group->Id());
    }

    // Test 5: GetGroup - simular resultado nulo
    TEST_F(GroupDelegateTest, GetGroup_NotFound_ReturnsNullptr) {
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
            .WillOnce(testing::Return(nullptr));

        auto result = groupDelegate->GetGroup("tournament-1", "group-1");

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(nullptr, result.value());
    }

    // Test 6: UpdateGroup - validar valor de Group transferido a GroupRepository, simular actualización válida
    TEST_F(GroupDelegateTest, UpdateGroup_Success) {
        auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
        tournament->Id() = "tournament-1";

        auto existingGroup = std::make_shared<domain::Group>("Old Name", "group-1");
        existingGroup->SetTournamentId("tournament-1");

        domain::Group capturedGroup;

        EXPECT_CALL(*tournamentRepositoryMock, ReadById("tournament-1"))
            .WillOnce(testing::Return(tournament));
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
            .WillOnce(testing::Return(existingGroup));
        EXPECT_CALL(*groupRepositoryMock, Update(testing::_))
            .WillOnce(testing::DoAll(testing::SaveArg<0>(&capturedGroup), testing::Return("group-1")));

        domain::Group group("Updated Name");
        group.SetId("group-1");

        auto result = groupDelegate->UpdateGroup("tournament-1", group);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ("Updated Name", capturedGroup.Name());
        EXPECT_EQ("tournament-1", capturedGroup.TournamentId());
    }

    // Test 7: UpdateGroup - simular ID no encontrado, regresar valor usando std::unexpected
    TEST_F(GroupDelegateTest, UpdateGroup_NotFound_ReturnsError) {
        auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
        tournament->Id() = "tournament-1";

        EXPECT_CALL(*tournamentRepositoryMock, ReadById("tournament-1"))
            .WillOnce(testing::Return(tournament));
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
            .WillOnce(testing::Return(nullptr));

        domain::Group group("Group Name");
        group.SetId("group-1");

        auto result = groupDelegate->UpdateGroup("tournament-1", group);

        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error(), testing::HasSubstr("doesn't exist"));
    }

    // Test 8: UpdateTeams - validar Team transferido a TeamRepository y GroupRepository, validar mensaje publicado, simular respuesta de ID
    TEST_F(GroupDelegateTest, UpdateTeams_Success) {
        auto group = std::make_shared<domain::Group>("Group A", "group-1");
        group->SetTournamentId("tournament-1");

        auto team1 = std::make_shared<domain::Team>("team-1", "Team A");
        auto team2 = std::make_shared<domain::Team>("team-2", "Team B");

        auto tournament = std::make_shared<domain::Tournament>();
        tournament->Id() = "tournament-1";
        tournament->Name() = "Tournament 1";
        tournament->Format().MaxTeamsPerGroup() = 8;

        EXPECT_CALL(*tournamentRepositoryMock, ReadById("tournament-1"))
            .WillOnce(testing::Return(tournament));
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
            .WillOnce(testing::Return(group));
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId("tournament-1", "team-1"))
            .WillOnce(testing::Return(nullptr));
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId("tournament-1", "team-2"))
            .WillOnce(testing::Return(nullptr));
        EXPECT_CALL(*teamRepositoryMock, ReadById("team-1")).WillOnce(testing::Return(team1));
        EXPECT_CALL(*teamRepositoryMock, ReadById("team-2")).WillOnce(testing::Return(team2));

        EXPECT_CALL(*groupRepositoryMock, UpdateGroupAddTeam("group-1", testing::_)).Times(2);
        EXPECT_CALL(*producerMock, SendMessage(testing::HasSubstr("team-1"), "tournament.team-add"));
        EXPECT_CALL(*producerMock, SendMessage(testing::HasSubstr("team-2"), "tournament.team-add"));

        std::vector<domain::Team> teams = {domain::Team("team-1", "Team A"), domain::Team("team-2", "Team B")};
        auto result = groupDelegate->UpdateTeams("tournament-1", "group-1", teams);

        ASSERT_TRUE(result.has_value());
    }

    // Test 9: UpdateTeams - simular que el equipo no existe, regresar valor usando std::unexpected
    TEST_F(GroupDelegateTest, UpdateTeams_TeamNotFound_ReturnsError) {
        auto group = std::make_shared<domain::Group>("Group A", "group-1");
        group->SetTournamentId("tournament-1");

        auto tournament = std::make_shared<domain::Tournament>();
        tournament->Id() = "tournament-1";
        tournament->Name() = "Tournament 1";
        tournament->Format().MaxTeamsPerGroup() = 8;

        EXPECT_CALL(*tournamentRepositoryMock, ReadById("tournament-1"))
            .WillOnce(testing::Return(tournament));
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
            .WillOnce(testing::Return(group));
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId("tournament-1", "team-1"))
            .WillOnce(testing::Return(nullptr));
        EXPECT_CALL(*teamRepositoryMock, ReadById("team-1")).WillOnce(testing::Return(nullptr));

        std::vector<domain::Team> teams = {domain::Team("team-1", "Team A")};
        auto result = groupDelegate->UpdateTeams("tournament-1", "group-1", teams);

        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error(), testing::HasSubstr("doesn't exist"));
    }

    // Test 10: UpdateTeams - simular que el grupo está lleno, regresar valor usando std::unexpected
    TEST_F(GroupDelegateTest, UpdateTeams_GroupFull_ReturnsError) {
        auto group = std::make_shared<domain::Group>("Group A", "group-1");
        group->SetTournamentId("tournament-1");
        group->MutableTeams().push_back(domain::Team{"existing-1", "Team 1"});
        group->MutableTeams().push_back(domain::Team{"existing-2", "Team 2"});

        auto tournament = std::make_shared<domain::Tournament>();
        tournament->Id() = "tournament-1";
        tournament->Name() = "Tournament 1";
        tournament->Format().MaxTeamsPerGroup() = 2; // Already at max

        EXPECT_CALL(*tournamentRepositoryMock, ReadById("tournament-1"))
            .WillOnce(testing::Return(tournament));
        EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
            .WillOnce(testing::Return(group));

        std::vector<domain::Team> teams = {domain::Team("team-1", "Team A")};
        auto result = groupDelegate->UpdateTeams("tournament-1", "group-1", teams);

        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error(), testing::HasSubstr("max capacity"));
    }
}