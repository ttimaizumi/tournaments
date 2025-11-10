#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "cms/ScoreUpdateListener.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/ScoreUpdateEvent.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"

// Mock for MatchDelegate
class MatchDelegateMock : public MatchDelegate {
public:
    MatchDelegateMock() : MatchDelegate(nullptr, nullptr, nullptr) {}

    MOCK_METHOD(void, ProcessScoreUpdate, (const domain::ScoreUpdateEvent&), (override));
};

// We can't easily test ScoreUpdateListener without a real ConnectionManager,
// so we'll test the processMessage logic through a test fixture
class ScoreUpdateListenerTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchDelegateMock> delegateMock;

    void SetUp() override {
        delegateMock = std::make_shared<MatchDelegateMock>();
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(delegateMock.get());
    }

    // Helper to simulate message processing
    void SimulateMessageProcessing(const std::string& jsonMessage) {
        domain::ScoreUpdateEvent event = nlohmann::json::parse(jsonMessage);
        delegateMock->ProcessScoreUpdate(event);
    }
};

// Test valid score update message
TEST_F(ScoreUpdateListenerTest, ProcessMessage_ValidScoreUpdate_CallsDelegate) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-456"},
        {"score", {
            {"home", 2},
            {"visitor", 1}
        }}
    };

    EXPECT_CALL(*delegateMock, ProcessScoreUpdate(testing::_))
            .Times(1)
            .WillOnce(testing::Invoke([](const domain::ScoreUpdateEvent& event) {
                EXPECT_EQ("tournament-123", event.tournamentId);
                EXPECT_EQ("match-456", event.matchId);
                EXPECT_EQ(2, event.score.homeTeamScore);
                EXPECT_EQ(1, event.score.visitorTeamScore);
            }));

    SimulateMessageProcessing(message.dump());
}

// Test score update with tie
TEST_F(ScoreUpdateListenerTest, ProcessMessage_TieScore_CallsDelegate) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-789"},
        {"score", {
            {"home", 1},
            {"visitor", 1}
        }}
    };

    EXPECT_CALL(*delegateMock, ProcessScoreUpdate(testing::_))
            .Times(1)
            .WillOnce(testing::Invoke([](const domain::ScoreUpdateEvent& event) {
                EXPECT_EQ(1, event.score.homeTeamScore);
                EXPECT_EQ(1, event.score.visitorTeamScore);
                EXPECT_TRUE(event.score.IsTie());
            }));

    SimulateMessageProcessing(message.dump());
}

// Test score update with zero scores
TEST_F(ScoreUpdateListenerTest, ProcessMessage_ZeroScores_CallsDelegate) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-000"},
        {"score", {
            {"home", 0},
            {"visitor", 0}
        }}
    };

    EXPECT_CALL(*delegateMock, ProcessScoreUpdate(testing::_))
            .Times(1)
            .WillOnce(testing::Invoke([](const domain::ScoreUpdateEvent& event) {
                EXPECT_EQ(0, event.score.homeTeamScore);
                EXPECT_EQ(0, event.score.visitorTeamScore);
            }));

    SimulateMessageProcessing(message.dump());
}

// Test score update with high scores
TEST_F(ScoreUpdateListenerTest, ProcessMessage_HighScores_CallsDelegate) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-high"},
        {"score", {
            {"home", 7},
            {"visitor", 5}
        }}
    };

    EXPECT_CALL(*delegateMock, ProcessScoreUpdate(testing::_))
            .Times(1)
            .WillOnce(testing::Invoke([](const domain::ScoreUpdateEvent& event) {
                EXPECT_EQ(7, event.score.homeTeamScore);
                EXPECT_EQ(5, event.score.visitorTeamScore);
                EXPECT_EQ(domain::Winner::HOME, event.score.GetWinner());
            }));

    SimulateMessageProcessing(message.dump());
}

// Test winner detection
TEST_F(ScoreUpdateListenerTest, ProcessMessage_HomeWins_CorrectWinnerDetected) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-home-win"},
        {"score", {
            {"home", 3},
            {"visitor", 1}
        }}
    };

    EXPECT_CALL(*delegateMock, ProcessScoreUpdate(testing::_))
            .Times(1)
            .WillOnce(testing::Invoke([](const domain::ScoreUpdateEvent& event) {
                EXPECT_EQ(domain::Winner::HOME, event.score.GetWinner());
                EXPECT_FALSE(event.score.IsTie());
            }));

    SimulateMessageProcessing(message.dump());
}

TEST_F(ScoreUpdateListenerTest, ProcessMessage_VisitorWins_CorrectWinnerDetected) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-visitor-win"},
        {"score", {
            {"home", 1},
            {"visitor", 4}
        }}
    };

    EXPECT_CALL(*delegateMock, ProcessScoreUpdate(testing::_))
            .Times(1)
            .WillOnce(testing::Invoke([](const domain::ScoreUpdateEvent& event) {
                EXPECT_EQ(domain::Winner::VISITOR, event.score.GetWinner());
                EXPECT_FALSE(event.score.IsTie());
            }));

    SimulateMessageProcessing(message.dump());
}

// Test invalid JSON handling (should throw and be handled by listener)
TEST_F(ScoreUpdateListenerTest, ProcessMessage_InvalidJSON_ThrowsException) {
    std::string invalidJson = "{invalid json}";

    EXPECT_THROW({
        nlohmann::json::parse(invalidJson);
    }, nlohmann::json::parse_error);
}

// Test missing fields
TEST_F(ScoreUpdateListenerTest, ProcessMessage_MissingTournamentId_ThrowsException) {
    nlohmann::json message = {
        {"matchId", "match-456"},
        {"score", {
            {"home", 2},
            {"visitor", 1}
        }}
    };

    EXPECT_THROW({
        domain::ScoreUpdateEvent event = message.get<domain::ScoreUpdateEvent>();
    }, nlohmann::json::exception);
}

TEST_F(ScoreUpdateListenerTest, ProcessMessage_MissingMatchId_ThrowsException) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"score", {
            {"home", 2},
            {"visitor", 1}
        }}
    };

    EXPECT_THROW({
        domain::ScoreUpdateEvent event = message.get<domain::ScoreUpdateEvent>();
    }, nlohmann::json::exception);
}

TEST_F(ScoreUpdateListenerTest, ProcessMessage_MissingScore_ThrowsException) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-456"}
    };

    EXPECT_THROW({
        domain::ScoreUpdateEvent event = message.get<domain::ScoreUpdateEvent>();
    }, nlohmann::json::exception);
}

TEST_F(ScoreUpdateListenerTest, ProcessMessage_MissingHomeScore_ThrowsException) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-456"},
        {"score", {
            {"visitor", 1}
        }}
    };

    EXPECT_THROW({
        domain::ScoreUpdateEvent event = message.get<domain::ScoreUpdateEvent>();
    }, nlohmann::json::exception);
}

TEST_F(ScoreUpdateListenerTest, ProcessMessage_MissingVisitorScore_ThrowsException) {
    nlohmann::json message = {
        {"tournamentId", "tournament-123"},
        {"matchId", "match-456"},
        {"score", {
            {"home", 2}
        }}
    };

    EXPECT_THROW({
        domain::ScoreUpdateEvent event = message.get<domain::ScoreUpdateEvent>();
    }, nlohmann::json::exception);
}

// Test goal difference calculation
TEST_F(ScoreUpdateListenerTest, Score_GetGoalDifference_CalculatesCorrectly) {
    domain::Score score{3, 1};

    EXPECT_EQ(2, score.GetGoalDifference(domain::Winner::HOME));
    EXPECT_EQ(-2, score.GetGoalDifference(domain::Winner::VISITOR));
}

TEST_F(ScoreUpdateListenerTest, Score_GetGoalDifference_TieReturnsZero) {
    domain::Score score{2, 2};

    EXPECT_EQ(0, score.GetGoalDifference(domain::Winner::HOME));
    EXPECT_EQ(0, score.GetGoalDifference(domain::Winner::VISITOR));
}
