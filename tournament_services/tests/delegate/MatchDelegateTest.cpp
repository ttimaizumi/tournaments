#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "domain/Match.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "delegate/MatchDelegate.hpp"

class MatchRepositoryMock : public IMatchRepository {
public:
    MOCK_METHOD(std::string, Create, (const domain::Match&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Match>, ReadById, (std::string), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, ReadAll, (), (override));
    MOCK_METHOD(std::string, Update, (const domain::Match&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByTournamentId, (const std::string_view&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindPlayedByTournamentId, (const std::string_view&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindPendingByTournamentId, (const std::string_view&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Match>, FindByTournamentIdAndMatchId, (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Match>, FindLastOpenMatch, (const std::string_view&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindMatchesByTournamentAndRound, (const std::string_view&, domain::Round), (override));
    MOCK_METHOD(bool, TournamentExists, (const std::string_view&), (override));
};

class QueueMessageProducerMock : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string&, const std::string&), (override));
};

class MatchDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchRepositoryMock> repositoryMock;
    std::shared_ptr<QueueMessageProducerMock> queueProducerMock;
    std::shared_ptr<MatchDelegate> matchDelegate;

    void SetUp() override {
        repositoryMock = std::make_shared<MatchRepositoryMock>();
        queueProducerMock = std::make_shared<QueueMessageProducerMock>();
        matchDelegate = std::make_shared<MatchDelegate>(repositoryMock, queueProducerMock);
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(repositoryMock.get());
        testing::Mock::VerifyAndClearExpectations(queueProducerMock.get());
    }
};

// GetMatch tests

TEST_F(MatchDelegateTest, GetMatch_ValidMatch_ReturnsMatch) {
    auto expectedMatch = std::make_shared<domain::Match>();
    expectedMatch->SetId("match-1");
    expectedMatch->SetTournamentId("tournament-1");
    expectedMatch->SetHomeTeamId("team-1");
    expectedMatch->SetHomeTeamName("Team A");
    expectedMatch->SetVisitorTeamId("team-2");
    expectedMatch->SetVisitorTeamName("Team B");
    expectedMatch->SetRound(domain::Round::REGULAR);

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(
            testing::Eq(std::string_view("tournament-1")),
            testing::Eq(std::string_view("match-1"))))
            .WillOnce(testing::Return(expectedMatch));

    auto result = matchDelegate->GetMatch("tournament-1", "match-1");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("match-1", result.value()->Id());
    EXPECT_EQ("Team A", result.value()->HomeTeamName());
}

TEST_F(MatchDelegateTest, GetMatch_MatchNotFound_ReturnsError) {
    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(
            testing::Eq(std::string_view("tournament-1")),
            testing::Eq(std::string_view("non-existent"))))
            .WillOnce(testing::Return(nullptr));

    auto result = matchDelegate->GetMatch("tournament-1", "non-existent");

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Match not found"));
}

TEST_F(MatchDelegateTest, GetMatch_RepositoryThrows_ReturnsError) {
    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Throw(std::runtime_error("Database error")));

    auto result = matchDelegate->GetMatch("tournament-1", "match-1");

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Error reading match"));
}

// GetMatches tests

TEST_F(MatchDelegateTest, GetMatches_AllFilter_ReturnsAllMatches) {
    auto match1 = std::make_shared<domain::Match>();
    match1->SetId("match-1");
    auto match2 = std::make_shared<domain::Match>();
    match2->SetId("match-2");
    std::vector<std::shared_ptr<domain::Match>> matches = {match1, match2};

    EXPECT_CALL(*repositoryMock, TournamentExists(testing::Eq(std::string_view("tournament-1"))))
            .WillOnce(testing::Return(true));
    EXPECT_CALL(*repositoryMock, FindByTournamentId(testing::Eq(std::string_view("tournament-1"))))
            .WillOnce(testing::Return(matches));

    auto result = matchDelegate->GetMatches("tournament-1", "all");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(2, result.value().size());
}

TEST_F(MatchDelegateTest, GetMatches_PlayedFilter_ReturnsPlayedMatches) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    domain::Score score{1, 2};
    match->SetScore(score);
    std::vector<std::shared_ptr<domain::Match>> matches = {match};

    EXPECT_CALL(*repositoryMock, TournamentExists(testing::Eq(std::string_view("tournament-1"))))
            .WillOnce(testing::Return(true));
    EXPECT_CALL(*repositoryMock, FindPlayedByTournamentId(testing::Eq(std::string_view("tournament-1"))))
            .WillOnce(testing::Return(matches));

    auto result = matchDelegate->GetMatches("tournament-1", "played");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value().size());
    EXPECT_TRUE(result.value()[0]->HasScore());
}

TEST_F(MatchDelegateTest, GetMatches_PendingFilter_ReturnsPendingMatches) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    std::vector<std::shared_ptr<domain::Match>> matches = {match};

    EXPECT_CALL(*repositoryMock, TournamentExists(testing::Eq(std::string_view("tournament-1"))))
            .WillOnce(testing::Return(true));
    EXPECT_CALL(*repositoryMock, FindPendingByTournamentId(testing::Eq(std::string_view("tournament-1"))))
            .WillOnce(testing::Return(matches));

    auto result = matchDelegate->GetMatches("tournament-1", "pending");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value().size());
    EXPECT_FALSE(result.value()[0]->HasScore());
}

TEST_F(MatchDelegateTest, GetMatches_TournamentNotFound_ReturnsError) {
    EXPECT_CALL(*repositoryMock, TournamentExists(testing::Eq(std::string_view("non-existent"))))
            .WillOnce(testing::Return(false));

    auto result = matchDelegate->GetMatches("non-existent", "all");

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Tournament not found"));
}

// UpdateMatchScore tests - Business rules

TEST_F(MatchDelegateTest, UpdateMatchScore_ValidScore_UpdatesAndPublishes) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetTournamentId("tournament-1");
    match->SetHomeTeamId("team-1");
    match->SetHomeTeamName("Team A");
    match->SetVisitorTeamId("team-2");
    match->SetVisitorTeamName("Team B");
    match->SetRound(domain::Round::REGULAR);

    domain::Score score{2, 1};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(
            testing::Eq(std::string_view("tournament-1")),
            testing::Eq(std::string_view("match-1"))))
            .WillOnce(testing::Return(match));
    EXPECT_CALL(*repositoryMock, Update(testing::_))
            .WillOnce(testing::Return("match-1"));
    EXPECT_CALL(*queueProducerMock, SendMessage(testing::_, testing::Eq(std::string("tournament.score-update"))))
            .Times(1);

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "match-1", score);

    ASSERT_TRUE(result.has_value());
}

TEST_F(MatchDelegateTest, UpdateMatchScore_NegativeHomeScore_ReturnsError) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetRound(domain::Round::REGULAR);

    domain::Score score{-1, 2};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Return(match));

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "match-1", score);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("non-negative"));
}

TEST_F(MatchDelegateTest, UpdateMatchScore_NegativeVisitorScore_ReturnsError) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetRound(domain::Round::REGULAR);

    domain::Score score{2, -1};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Return(match));

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "match-1", score);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("non-negative"));
}

TEST_F(MatchDelegateTest, UpdateMatchScore_TieInRegularMatch_Succeeds) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetRound(domain::Round::REGULAR);

    domain::Score score{1, 1};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Return(match));
    EXPECT_CALL(*repositoryMock, Update(testing::_))
            .WillOnce(testing::Return("match-1"));
    EXPECT_CALL(*queueProducerMock, SendMessage(testing::_, testing::_))
            .Times(1);

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "match-1", score);

    ASSERT_TRUE(result.has_value());
}

TEST_F(MatchDelegateTest, UpdateMatchScore_TieInEighths_ReturnsError) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetRound(domain::Round::EIGHTHS);

    domain::Score score{1, 1};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Return(match));

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "match-1", score);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Tie not allowed"));
}

TEST_F(MatchDelegateTest, UpdateMatchScore_TieInQuarters_ReturnsError) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetRound(domain::Round::QUARTERS);

    domain::Score score{2, 2};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Return(match));

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "match-1", score);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Tie not allowed"));
}

TEST_F(MatchDelegateTest, UpdateMatchScore_TieInSemis_ReturnsError) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetRound(domain::Round::SEMIS);

    domain::Score score{0, 0};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Return(match));

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "match-1", score);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Tie not allowed"));
}

TEST_F(MatchDelegateTest, UpdateMatchScore_TieInFinal_ReturnsError) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetRound(domain::Round::FINAL);

    domain::Score score{3, 3};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Return(match));

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "match-1", score);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Tie not allowed"));
}

TEST_F(MatchDelegateTest, UpdateMatchScore_MatchNotFound_ReturnsError) {
    domain::Score score{1, 2};

    EXPECT_CALL(*repositoryMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
            .WillOnce(testing::Return(nullptr));

    auto result = matchDelegate->UpdateMatchScore("tournament-1", "non-existent", score);

    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Match not found"));
}
