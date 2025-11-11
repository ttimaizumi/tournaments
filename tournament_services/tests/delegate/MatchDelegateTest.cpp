//
// Created by edgar on 11/10/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <nlohmann/json.hpp>

#include "delegate/MatchDelegate.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"

using nlohmann::json;
using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class MockMatchRepository : public IMatchRepository
{
public:
    MOCK_METHOD(std::vector<json>,
                FindByTournament,
                (std::string_view, std::optional<std::string>), (override));

    MOCK_METHOD(std::optional<json>,
                FindByTournamentAndId,
                (std::string_view, std::string_view), (override));

    MOCK_METHOD(bool,
                UpdateScore,
                (std::string_view, std::string_view,
                 const json&, std::string), (override));
};

class MockProducer : public IQueueMessageProducer
{
public:
    MOCK_METHOD(void, SendMessage, (std::string_view, std::string_view), (override));
};

TEST(MatchDelegateTest, UpdateScore_Success_SingleElimination)
{
    auto repo    = std::make_shared<NiceMock<MockMatchRepository>>();
    auto producer = std::make_shared<NiceMock<MockProducer>>();

    MatchDelegate delegate(repo, producer);

    json doc{
        {"id", "m1"},
        {"tournamentId", "t1"},
        {"round", "quarterfinal"}   // eliminacion sencilla
    };

    EXPECT_CALL(*repo, FindByTournamentAndId("t1", "m1"))
        .WillOnce(Return(doc));

    EXPECT_CALL(*repo,
                UpdateScore("t1", "m1",
                            testing::Property(&json::dump, testing::_),
                            "played"))
        .WillOnce(Return(true));

    EXPECT_CALL(*producer, SendMessage("m1", "match.score-recorded"))
        .Times(1);

    auto r = delegate.UpdateScore("t1", "m1", 1, 0);
    EXPECT_TRUE(r.has_value());
}

TEST(MatchDelegateTest, UpdateScore_Fails_OnDraw_InElimination)
{
    auto repo    = std::make_shared<NiceMock<MockMatchRepository>>();
    auto producer = std::make_shared<NiceMock<MockProducer>>();

    MatchDelegate delegate(repo, producer);

    json doc{
        {"id", "m1"},
        {"tournamentId", "t1"},
        {"round", "quarterfinal"}   // eliminacion sencilla
    };

    EXPECT_CALL(*repo, FindByTournamentAndId("t1", "m1"))
        .WillOnce(Return(doc));

    auto r = delegate.UpdateScore("t1", "m1", 1, 1); // empate
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "invalid-score");
}

TEST(MatchDelegateTest, UpdateScore_NotFound)
{
    auto repo    = std::make_shared<NiceMock<MockMatchRepository>>();
    auto producer = std::make_shared<NiceMock<MockProducer>>();

    MatchDelegate delegate(repo, producer);

    EXPECT_CALL(*repo, FindByTournamentAndId("t1", "m1"))
        .WillOnce(Return(std::optional<json>{}));

    auto r = delegate.UpdateScore("t1", "m1", 1, 0);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "match-not-found");
}
