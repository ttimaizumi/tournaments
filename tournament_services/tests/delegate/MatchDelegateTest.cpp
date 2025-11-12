#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "delegate/MatchDelegate.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"

using nlohmann::json;
using ::testing::_;
using ::testing::Return;

class MockMatchRepository : public IMatchRepository {
public:
    MOCK_METHOD(std::vector<json>, FindByTournament,
        (std::string_view, std::optional<std::string>), (override));

    MOCK_METHOD(std::optional<json>, FindByTournamentAndId,
        (std::string_view, std::string_view), (override));

    MOCK_METHOD(std::optional<std::string>, Create,
        (const json&), (override));

    MOCK_METHOD(bool, UpdateScore,
        (std::string_view, std::string_view, const json&, std::string), (override));

    MOCK_METHOD(bool, UpdateParticipants,
        (std::string_view, std::string_view,
         std::optional<std::string>, std::optional<std::string>), (override));
};

class MockProducer : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage,
        (const std::string_view&, const std::string_view&), (override));
};

TEST(MatchDelegateTest, CreateMatch_SendsCreatedEvent) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    json body{{"bracket","winners"},{"round",1}};

    EXPECT_CALL(*repo, Create(_))
        .WillOnce(Return(std::optional<std::string>{"m1"}));
    EXPECT_CALL(*prod, SendMessage("m1", "match.created"))
        .Times(1);

    auto r = d.CreateMatch("t1", body);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->at("id"), "m1");
}

TEST(MatchDelegateTest, UpdateScore_Advances_NoTie) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    json doc{
        {"id","m1"},
        {"tournamentId","t1"},
        {"homeTeamId","A"},
        {"visitorTeamId","B"},
        {"status","scheduled"},
        {"score", json{{"home",0},{"visitor",0}}},
        {"advancement", json{
            {"winner", json{{"matchId","m2"},{"slot","home"}}},
            {"loser",  json{{"matchId","m3"},{"slot","visitor"}}}
        }}
    };

    // El delegate debe encontrar el match original
    EXPECT_CALL(*repo, FindByTournamentAndId("t1","m1"))
        .WillOnce(Return(std::optional<json>{doc}));

    // Debe guardar el score como "played"
    EXPECT_CALL(*repo, UpdateScore("t1","m1",_, "played"))
        .WillOnce(Return(true));

    // Ganador A -> m2.home ; Perdedor B -> m3.visitor
    // Usamos '_' en los opcionales para evitar el problema con std::nullopt.
    EXPECT_CALL(*repo,
        UpdateParticipants("t1", "m2",
                           std::optional<std::string>("A"), _))
        .WillOnce(Return(true));

    EXPECT_CALL(*repo,
        UpdateParticipants("t1", "m3",
                           _, std::optional<std::string>("B")))
        .WillOnce(Return(true));

    // Eventos esperados
    EXPECT_CALL(*prod, SendMessage("m2", "match.advanced")).Times(1);
    EXPECT_CALL(*prod, SendMessage("m3", "match.advanced")).Times(1);
    EXPECT_CALL(*prod, SendMessage("m1", "match.score-recorded")).Times(1);

    auto r = d.UpdateScore("t1","m1", 2, 1);
    EXPECT_TRUE(r.has_value());
}
