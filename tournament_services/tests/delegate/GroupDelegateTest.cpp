#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <expected>

#include "domain/Group.hpp"
#include "delegate/GroupDelegate.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "exception/Duplicate.hpp"
#include "exception/NotFound.hpp"
#include "exception/InvalidFormat.hpp"

class MockGroupRepository : public IGroupRepository {
    public:
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD(std::string, Create, (const domain::Group& entity), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId, (const std::string_view& tournamentId, const std::string_view& teamId), (override));   
    MOCK_METHOD(void, UpdateGroupAddTeam, (const std::string_view& groupId, const std::shared_ptr<domain::Team> & team), (override));
};

class MockTournamentRepository : public TournamentRepository {
public:
    MockTournamentRepository() : TournamentRepository(nullptr) {}
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
};

class MockTeamRepository : public TeamRepository {
public:
    MockTeamRepository() : TeamRepository(nullptr) {}
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view id), (override));
    MOCK_METHOD(std::string_view, Create, (const domain::Team& entity), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team& entity), (override));
    MOCK_METHOD(void, Delete, (std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
};

class GroupDelegateTest : public ::testing::Test {
    protected:
    std::shared_ptr<MockTournamentRepository> mockTournamentRepository;
    std::shared_ptr<MockGroupRepository> mockGroupRepository;
    std::shared_ptr<MockTeamRepository> mockTeamRepository;
    std::shared_ptr<GroupDelegate> groupDelegate;

    void SetUp() override {
        mockTournamentRepository = std::make_shared<MockTournamentRepository>();
        mockGroupRepository = std::make_shared<MockGroupRepository>();
        mockTeamRepository = std::make_shared<MockTeamRepository>();
        groupDelegate = std::make_shared<GroupDelegate>(mockTournamentRepository, mockGroupRepository, mockTeamRepository);
    }
};

//======================= GetGroups TESTS ========================
// Validar retorno exitoso con múltiples grupos
TEST_F(GroupDelegateTest, GetGroups_ReturnsMultipleGroups) {
    std::vector<std::shared_ptr<domain::Group>> groups = {
        std::make_shared<domain::Group>(domain::Group{"Group 1", "group-1"}),
        std::make_shared<domain::Group>(domain::Group{"Group 2", "group-2"}),
        std::make_shared<domain::Group>(domain::Group{"Group 3", "group-3"})
    };
    groups[0]->TournamentId() = "tournament-id";
    groups[1]->TournamentId() = "tournament-id";
    groups[2]->TournamentId() = "tournament-id";

    EXPECT_CALL(*mockGroupRepository, FindByTournamentId(testing::Eq(std::string("tournament-id"))))
        .WillOnce(testing::Return(groups));

    auto result = groupDelegate->GetGroups("tournament-id");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 3);
    EXPECT_EQ((*result)[0]->Id(), "group-1");
    EXPECT_EQ((*result)[0]->Name(), "Group 1");
    EXPECT_EQ((*result)[1]->Id(), "group-2");
    EXPECT_EQ((*result)[1]->Name(), "Group 2");
    EXPECT_EQ((*result)[2]->Id(), "group-3");
    EXPECT_EQ((*result)[2]->Name(), "Group 3");
}

// Validar retorno exitoso con ningún grupo
TEST_F(GroupDelegateTest, GetGroups_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Group>> emptyGroups;

    EXPECT_CALL(*mockGroupRepository, FindByTournamentId(testing::Eq(std::string("tournament-id"))))
        .WillOnce(testing::Return(emptyGroups));

    auto result = groupDelegate->GetGroups("tournament-id");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

// Tests for invalid/non-existent tournament IDs

// Validar que GetGroups maneja torneo inexistente
TEST_F(GroupDelegateTest, GetGroups_NonExistentTournamentId_ThrowsException) {
    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("non-existent-tournament"))))
        .WillOnce(testing::Throw(NotFoundException("Tournament not found")));

    EXPECT_THROW(groupDelegate->GetGroups("non-existent-tournament"), NotFoundException);
}

// Validar que GetGroups maneja ID de torneo con formato inválido
TEST_F(GroupDelegateTest, GetGroups_InvalidTournamentIdFormat_ThrowsException) {
    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string(""))))
        .WillOnce(testing::Throw(InvalidFormatException("Invalid tournament ID format")));

    EXPECT_THROW(groupDelegate->GetGroups(""), InvalidFormatException);
    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("invalid@id#format"))))
        .WillOnce(testing::Throw(InvalidFormatException("Invalid tournament ID format")));

    EXPECT_THROW(groupDelegate->GetGroups("invalid@id#format"), InvalidFormatException);
}

// GetGroup Tests

// Validar que GetGroup funciona correctamente con IDs válidos
TEST_F(GroupDelegateTest, GetGroup_ValidIds_ReturnsGroup) {
    auto group = std::make_shared<domain::Group>(domain::Group{"Group 1", "group-1"});
    group->TournamentId() = "tournament-id";

    EXPECT_CALL(*mockGroupRepository, FindByTournamentIdAndGroupId(
        testing::Eq(std::string("tournament-id")), 
        testing::Eq(std::string("group-1"))))
        .WillOnce(testing::Return(group));

    auto result = groupDelegate->GetGroup("tournament-id", "group-1");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->Id(), "group-1");
    EXPECT_EQ((*result)->Name(), "Group 1");
    EXPECT_EQ((*result)->TournamentId(), "tournament-id");
}

// Validar que GetGroup maneja grupo inexistente
TEST_F(GroupDelegateTest, GetGroup_NonExistentGroupId_ReturnsError) {
    EXPECT_CALL(*mockGroupRepository, FindByTournamentIdAndGroupId(
        testing::Eq(std::string("tournament-id")), 
        testing::Eq(std::string("non-existent-group"))))
        .WillOnce(testing::Throw(std::runtime_error("Group not found")));

    auto result = groupDelegate->GetGroup("tournament-id", "non-existent-group");

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Error when reading to DB"));
}

// Validar que GetGroup maneja excepciones de base de datos
TEST_F(GroupDelegateTest, GetGroup_DatabaseError_ReturnsError) {
    EXPECT_CALL(*mockGroupRepository, FindByTournamentIdAndGroupId(
        testing::Eq(std::string("tournament-id")), 
        testing::Eq(std::string("group-1"))))
        .WillOnce(testing::Throw(std::runtime_error("Database connection error")));

    auto result = groupDelegate->GetGroup("tournament-id", "group-1");

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Error when reading to DB"));
}

// CreateGroup Tests

// Validar que CreateGroup funciona correctamente
TEST_F(GroupDelegateTest, CreateGroup_ValidData_ReturnsSuccess) {
    domain::Group group{"Group 1", "group-1"};
    auto tournament = std::make_shared<domain::Tournament>(domain::Tournament{"Tournament Name"});

    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("tournament-id"))))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepository, Create(testing::_))
        .WillOnce(testing::Return("group-1"));

    auto result = groupDelegate->CreateGroup("tournament-id", group);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "group-1");
}

// Validar que CreateGroup maneja torneo inexistente
TEST_F(GroupDelegateTest, CreateGroup_NonExistentTournamentId_ThrowsException) {
    domain::Group group{"Group 1", "group-1"};
    
    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("non-existent-tournament"))))
        .WillOnce(testing::Throw(NotFoundException("Tournament not found")));

    EXPECT_THROW(groupDelegate->CreateGroup("non-existent-tournament", group), NotFoundException);
}

// Test for enhanced group creation with validation
TEST_F(GroupDelegateTest, CreateGroup_ValidData_ValidatesGroupPassedToRepository) {
    domain::Group group{"Group A", "group-a"};
    auto tournament = std::make_shared<domain::Tournament>(domain::Tournament{"Tournament 1"});
    tournament->Id() = "tournament-123";

    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("tournament-123"))))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*mockGroupRepository, Create(testing::_))
        .WillOnce(testing::DoAll(
            testing::WithArg<0>(testing::Invoke([](const domain::Group& g) {
                EXPECT_EQ(g.TournamentId(), "tournament-123");
                EXPECT_EQ(g.Name(), "Group A");
                EXPECT_EQ(g.Id(), "group-a");
            })),
            testing::Return("generated-group-id")
        ));

    auto result = groupDelegate->CreateGroup("tournament-123", group);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "generated-group-id");
}

// Test for group creation with duplicate error using std::expected
TEST_F(GroupDelegateTest, CreateGroup_DuplicateGroup_ReturnsExpectedError) {
    domain::Group group{"Group A", "group-a"};
    auto tournament = std::make_shared<domain::Tournament>(domain::Tournament{"Tournament 1"});

    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("tournament-123"))))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*mockGroupRepository, Create(testing::_))
        .WillOnce(testing::DoAll(
            testing::WithArg<0>(testing::Invoke([](const domain::Group& g) {
                EXPECT_EQ(g.TournamentId(), "tournament-123");
                EXPECT_EQ(g.Name(), "Group A");
            })),
            testing::Throw(DuplicateException("Group already exists"))
        ));

    EXPECT_THROW(groupDelegate->CreateGroup("tournament-123", group), DuplicateException);
}

// Test for group creation with max teams error using std::expected
// TEST_F(GroupDelegateTest, CreateGroup_MaxTeamsReached_ReturnsExpectedError) {
//     domain::Group group{"Group A", "group-a"};
//     auto tournament = std::make_shared<domain::Tournament>(domain::Tournament{"Tournament 1"});

    // EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("tournament-123"))))
    //     .WillOnce(testing::Return(tournament));

    // // Simulate max capacity error during creation
    // EXPECT_CALL(*mockGroupRepository, Create(testing::_))
    //     .WillOnce(testing::DoAll(
    //         testing::WithArg<0>(testing::Invoke([](const domain::Group& g) {
    //             EXPECT_EQ(g.TournamentId(), "tournament-123");
    //             EXPECT_EQ(g.Name(), "Group A");
    //         })),
    //         testing::Throw(std::runtime_error("Maximum number of teams reached"))
    //     ));

    // auto result = groupDelegate->CreateGroup("tournament-123", group);

//     ASSERT_FALSE(result.has_value());
//     EXPECT_THAT(result.error(), testing::HasSubstr("Error creating group"));
// }

// Enhanced GetGroup Tests with proper parameter validation

// Test GetGroup with successful result and parameter validation
TEST_F(GroupDelegateTest, GetGroup_ValidIds_ValidatesParameters) {
    auto group = std::make_shared<domain::Group>(domain::Group{"Group A", "group-123"});
    group->TournamentId() = "tournament-456";

    auto tournament = std::make_shared<domain::Tournament>(domain::Tournament{"Tournament 1"});
    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("tournament-456"))))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*mockGroupRepository, FindByTournamentIdAndGroupId(
        testing::Eq(std::string("tournament-456")), 
        testing::Eq(std::string("group-123"))))
        .WillOnce(testing::Return(group));

    auto result = groupDelegate->GetGroup("tournament-456", "group-123");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->Id(), "group-123");
    EXPECT_EQ((*result)->Name(), "Group A");
    EXPECT_EQ((*result)->TournamentId(), "tournament-456");
}

// Test GetGroup with null result and parameter validation
TEST_F(GroupDelegateTest, GetGroup_ValidIds_ReturnsNullResult) {
    auto tournament = std::make_shared<domain::Tournament>(domain::Tournament{"Tournament 1"});
    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("tournament-456"))))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*mockGroupRepository, FindByTournamentIdAndGroupId(
        testing::Eq(std::string("tournament-456")), 
        testing::Eq(std::string("non-existent-group"))))
        .WillOnce(testing::Return(nullptr));

    auto result = groupDelegate->GetGroup("tournament-456", "non-existent-group");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), nullptr);
}

// UpdateGroup Tests

// Test UpdateGroup with valid data and parameter validation
TEST_F(GroupDelegateTest, UpdateGroup_ValidData_ValidatesParameters) {
    domain::Group inputGroup{"Updated Group", "original-id"};
    auto existingGroup = std::make_shared<domain::Group>(domain::Group{"Existing Group", "group-789"});
    auto tournament = std::make_shared<domain::Tournament>(domain::Tournament{"Tournament 1"});

    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("tournament-456"))))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*mockGroupRepository, FindByTournamentIdAndGroupId(
        testing::Eq(std::string("tournament-456")), 
        testing::Eq(std::string("group-789"))))
        .WillOnce(testing::Return(existingGroup));

    EXPECT_CALL(*mockGroupRepository, Update(testing::_))
        .WillOnce(testing::DoAll(
            testing::WithArg<0>(testing::Invoke([](const domain::Group& g) {
                EXPECT_EQ(g.Id(), "group-789");  // Should be the groupId parameter, not original
                EXPECT_EQ(g.TournamentId(), "tournament-456");
                EXPECT_EQ(g.Name(), "Updated Group");
            })),
            testing::Return("group-789")
        ));

    auto result = groupDelegate->UpdateGroup("tournament-456", inputGroup, "group-789");

    ASSERT_TRUE(result.has_value());
}

// Test UpdateGroup with not found error using std::expected
TEST_F(GroupDelegateTest, UpdateGroup_GroupNotFound_ReturnsExpectedError) {
    domain::Group inputGroup{"Updated Group", "original-id"};
    auto tournament = std::make_shared<domain::Tournament>(domain::Tournament{"Tournament 1"});

    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("tournament-456"))))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*mockGroupRepository, FindByTournamentIdAndGroupId(
        testing::Eq(std::string("tournament-456")), 
        testing::Eq(std::string("non-existent-group"))))
        .WillOnce(testing::Throw(NotFoundException("Group not found")));

    EXPECT_THROW(groupDelegate->UpdateGroup("tournament-456", inputGroup, "non-existent-group"), NotFoundException);
}

// Test UpdateGroup with tournament not found error using std::expected
TEST_F(GroupDelegateTest, UpdateGroup_TournamentNotFound_ReturnsExpectedError) {
    domain::Group inputGroup{"Updated Group", "group-id"};

    EXPECT_CALL(*mockTournamentRepository, ReadById(testing::Eq(std::string("non-existent-tournament"))))
        .WillOnce(testing::Throw(NotFoundException("Tournament not found")));

    EXPECT_THROW(groupDelegate->UpdateGroup("non-existent-tournament", inputGroup, "group-id"), NotFoundException);
}

