#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Match.hpp"
#include "domain/Utilities.hpp"
#include "delegate/IMatchDelegate.hpp"
#include "controller/MatchController.hpp"

class MatchDelegateMock : public IMatchDelegate {
public:
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Match>, std::string>),
                GetMatch, (std::string_view tournamentId, std::string_view matchId), (override));

    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>),
                GetMatches, (std::string_view tournamentId, std::string_view filter), (override));

    MOCK_METHOD((std::expected<void, std::string>),
                UpdateMatchScore, (std::string_view tournamentId, std::string_view matchId, const domain::Score& score), (override));
};

class MatchControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchDelegateMock> matchDelegateMock;
    std::shared_ptr<MatchController> matchController;

    void SetUp() override {
        matchDelegateMock = std::make_shared<MatchDelegateMock>();
        matchController = std::make_shared<MatchController>(matchDelegateMock);
    }

    void TearDown() override {
    }
};

// GET /tournaments/<id>/matches tests

TEST_F(MatchControllerTest, GetMatches_EmptyList_Returns200WithEmptyArray) {
    std::vector<std::shared_ptr<domain::Match>> emptyMatches;

    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::Eq(std::string("tournament-1")), testing::Eq(std::string("all"))))
            .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(emptyMatches)));

    crow::request req;
    crow::response response = matchController->getMatches(req, "tournament-1");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_TRUE(jsonResponse.is_array());
    EXPECT_EQ(0, jsonResponse.size());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

TEST_F(MatchControllerTest, GetMatches_WithMatches_Returns200WithArray) {
    auto match1 = std::make_shared<domain::Match>();
    match1->SetId("match-1");
    match1->SetTournamentId("tournament-1");
    match1->SetHomeTeamId("team-1");
    match1->SetHomeTeamName("Team A");
    match1->SetVisitorTeamId("team-2");
    match1->SetVisitorTeamName("Team B");
    match1->SetRound(domain::Round::REGULAR);

    auto match2 = std::make_shared<domain::Match>();
    match2->SetId("match-2");
    match2->SetTournamentId("tournament-1");
    match2->SetHomeTeamId("team-3");
    match2->SetHomeTeamName("Team C");
    match2->SetVisitorTeamId("team-4");
    match2->SetVisitorTeamName("Team D");
    match2->SetRound(domain::Round::REGULAR);
    domain::Score score{1, 2};
    match2->SetScore(score);

    std::vector<std::shared_ptr<domain::Match>> matches = {match1, match2};

    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::Eq(std::string("tournament-1")), testing::Eq(std::string("all"))))
            .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches)));

    crow::request req;
    crow::response response = matchController->getMatches(req, "tournament-1");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(2, jsonResponse.size());
    EXPECT_EQ("Team A", jsonResponse[0]["home"]["name"].get<std::string>());
    EXPECT_EQ("Team B", jsonResponse[0]["visitor"]["name"].get<std::string>());
    EXPECT_EQ("regular", jsonResponse[0]["round"].get<std::string>());
    EXPECT_FALSE(jsonResponse[0].contains("score"));

    EXPECT_EQ("Team C", jsonResponse[1]["home"]["name"].get<std::string>());
    EXPECT_EQ("Team D", jsonResponse[1]["visitor"]["name"].get<std::string>());
    EXPECT_EQ("regular", jsonResponse[1]["round"].get<std::string>());
    EXPECT_TRUE(jsonResponse[1].contains("score"));
    EXPECT_EQ(1, jsonResponse[1]["score"]["home"].get<int>());
    EXPECT_EQ(2, jsonResponse[1]["score"]["visitor"].get<int>());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

TEST_F(MatchControllerTest, GetMatches_TournamentNotFound_Returns404) {
    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::Eq(std::string("non-existent")), testing::Eq(std::string("all"))))
            .WillOnce(testing::Return(std::unexpected("Tournament not found")));

    crow::request req;
    crow::response response = matchController->getMatches(req, "non-existent");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(MatchControllerTest, GetMatches_WithFilterPlayed_Returns200WithPlayedMatches) {
    auto match1 = std::make_shared<domain::Match>();
    match1->SetId("match-1");
    match1->SetHomeTeamId("team-1");
    match1->SetHomeTeamName("Team A");
    match1->SetVisitorTeamId("team-2");
    match1->SetVisitorTeamName("Team B");
    match1->SetRound(domain::Round::REGULAR);
    domain::Score score{2, 1};
    match1->SetScore(score);

    std::vector<std::shared_ptr<domain::Match>> matches = {match1};

    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::Eq(std::string("tournament-1")), testing::Eq(std::string("played"))))
            .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches)));

    crow::request req;
    req.url_params = crow::query_string("?showMatches=played");

    crow::response response = matchController->getMatches(req, "tournament-1");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(1, jsonResponse.size());
    EXPECT_TRUE(jsonResponse[0].contains("score"));
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

TEST_F(MatchControllerTest, GetMatches_WithFilterPending_Returns200WithPendingMatches) {
    auto match1 = std::make_shared<domain::Match>();
    match1->SetId("match-1");
    match1->SetHomeTeamId("team-1");
    match1->SetHomeTeamName("Team A");
    match1->SetVisitorTeamId("team-2");
    match1->SetVisitorTeamName("Team B");
    match1->SetRound(domain::Round::REGULAR);

    std::vector<std::shared_ptr<domain::Match>> matches = {match1};

    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::Eq(std::string("tournament-1")), testing::Eq(std::string("pending"))))
            .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches)));

    crow::request req;
    req.url_params = crow::query_string("?showMatches=pending");

    crow::response response = matchController->getMatches(req, "tournament-1");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(1, jsonResponse.size());
    EXPECT_FALSE(jsonResponse[0].contains("score"));
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

// GET /tournaments/<id>/matches/<matchId> tests

TEST_F(MatchControllerTest, GetMatch_ValidId_Returns200WithMatch) {
    auto match = std::make_shared<domain::Match>();
    match->SetId("match-1");
    match->SetTournamentId("tournament-1");
    match->SetHomeTeamId("1223445");
    match->SetHomeTeamName("Equipo1");
    match->SetVisitorTeamId("09887766");
    match->SetVisitorTeamName("Equipo2");
    match->SetRound(domain::Round::REGULAR);
    domain::Score score{1, 2};
    match->SetScore(score);

    EXPECT_CALL(*matchDelegateMock, GetMatch(testing::Eq(std::string("tournament-1")), testing::Eq(std::string("match-1"))))
            .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match)));

    crow::response response = matchController->getMatch("tournament-1", "match-1");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("1223445", jsonResponse["home"]["id"].get<std::string>());
    EXPECT_EQ("Equipo1", jsonResponse["home"]["name"].get<std::string>());
    EXPECT_EQ("09887766", jsonResponse["visitor"]["id"].get<std::string>());
    EXPECT_EQ("Equipo2", jsonResponse["visitor"]["name"].get<std::string>());
    EXPECT_EQ("regular", jsonResponse["round"].get<std::string>());
    EXPECT_EQ(1, jsonResponse["score"]["home"].get<int>());
    EXPECT_EQ(2, jsonResponse["score"]["visitor"].get<int>());
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

TEST_F(MatchControllerTest, GetMatch_NotFound_Returns404) {
    EXPECT_CALL(*matchDelegateMock, GetMatch(testing::Eq(std::string("tournament-1")), testing::Eq(std::string("non-existent"))))
            .WillOnce(testing::Return(std::unexpected("Match not found")));

    crow::response response = matchController->getMatch("tournament-1", "non-existent");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// PATCH /tournaments/<id>/matches/<matchId> tests

TEST_F(MatchControllerTest, UpdateMatchScore_ValidScore_Returns204) {
    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(testing::Eq(std::string("tournament-1")),
                                                       testing::Eq(std::string("match-1")),
                                                       testing::_))
            .WillOnce(testing::Return(std::expected<void, std::string>()));

    crow::request req;
    req.body = R"({"score": {"home": 1, "visitor": 2}})";

    crow::response response = matchController->updateMatchScore(req, "tournament-1", "match-1");

    EXPECT_EQ(crow::NO_CONTENT, response.code);
}

TEST_F(MatchControllerTest, UpdateMatchScore_MatchNotFound_Returns404) {
    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(testing::Eq(std::string("tournament-1")),
                                                       testing::Eq(std::string("non-existent")),
                                                       testing::_))
            .WillOnce(testing::Return(std::unexpected("Match not found")));

    crow::request req;
    req.body = R"({"score": {"home": 1, "visitor": 2}})";

    crow::response response = matchController->updateMatchScore(req, "tournament-1", "non-existent");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(MatchControllerTest, UpdateMatchScore_TieInPlayoffs_Returns422) {
    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(testing::Eq(std::string("tournament-1")),
                                                       testing::Eq(std::string("match-1")),
                                                       testing::_))
            .WillOnce(testing::Return(std::unexpected("Tie not allowed in playoff matches")));

    crow::request req;
    req.body = R"({"score": {"home": 1, "visitor": 1}})";

    crow::response response = matchController->updateMatchScore(req, "tournament-1", "match-1");

    EXPECT_EQ(422, response.code);
}
