#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "domain/Match.hpp"
#include "controller/MatchController.hpp"
#include "delegate/IMatchDelegate.hpp"

using ::testing::_;
using ::testing::Return;

// Mock del delegate
class MockMatchDelegate : public IMatchDelegate {
public:
  MOCK_METHOD((std::expected<std::string,int>), CreateMatch, (const std::string&, const Match&), (override));
  MOCK_METHOD((std::optional<Match>),           FindMatchById, (const std::string&), (override));
  MOCK_METHOD((std::vector<Match>),             ListMatchesByGroup, (const std::string&, const std::string&), (override));
  MOCK_METHOD((std::expected<void,int>),        UpdateScore, (const std::string&, int, int), (override));
  MOCK_METHOD((std::expected<void,int>),        FinishMatch, (const std::string&), (override));
};

//Tests de MatchController

TEST(MatchControllerTest, PostMatch_201_Location) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);
  Match m{"", "T1", "GA", "TEAM1", "TEAM2"};

  EXPECT_CALL(mock, CreateMatch("T1", _)).WillOnce(Return(std::expected<std::string,int>{"MATCH-001"}));

  std::string loc;
  EXPECT_EQ(201, ctrl.PostMatch("T1", m, &loc));
  EXPECT_EQ("MATCH-001", loc);
}

TEST(MatchControllerTest, PostMatch_409_Conflict) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);
  Match m{"", "T1", "GA", "TEAM1", "TEAM2"};

  EXPECT_CALL(mock, CreateMatch("T1", _)).WillOnce(Return(std::unexpected{409}));

  EXPECT_EQ(409, ctrl.PostMatch("T1", m, nullptr));
}

TEST(MatchControllerTest, GetMatch_200_WithObject) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);
  Match expected{"M1", "T1", "GA", "TEAM1", "TEAM2"};

  EXPECT_CALL(mock, FindMatchById("M1")).WillOnce(Return(expected));

  auto [code, match] = ctrl.GetMatch("M1");
  EXPECT_EQ(200, code);
  ASSERT_TRUE(match);
  EXPECT_EQ("TEAM1", match->homeTeamId);
}

TEST(MatchControllerTest, GetMatch_404_NotFound) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);

  EXPECT_CALL(mock, FindMatchById("NOPE")).WillOnce(Return(std::nullopt));

  auto [code, match] = ctrl.GetMatch("NOPE");
  EXPECT_EQ(404, code);
  EXPECT_FALSE(match);
}

TEST(MatchControllerTest, GetMatchesByGroup_200_WithList) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);
  std::vector<Match> matches = {
    Match{"M1", "T1", "GA", "T1", "T2"},
    Match{"M2", "T1", "GA", "T3", "T4"}
  };

  EXPECT_CALL(mock, ListMatchesByGroup("T1", "GA")).WillOnce(Return(matches));

  auto [code, list] = ctrl.GetMatchesByGroup("T1", "GA");
  EXPECT_EQ(200, code);
  EXPECT_EQ(2u, list.size());
}

TEST(MatchControllerTest, PatchScore_200_ValidScore) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);

  EXPECT_CALL(mock, UpdateScore("M1", 2, 1)).WillOnce(Return(std::expected<void,int>{}));

  EXPECT_EQ(200, ctrl.PatchScore("M1", 2, 1));
}

TEST(MatchControllerTest, PatchScore_400_InvalidScore) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);

  EXPECT_CALL(mock, UpdateScore("M1", 15, 1)).WillOnce(Return(std::unexpected{400}));

  EXPECT_EQ(400, ctrl.PatchScore("M1", 15, 1));
}

TEST(MatchControllerTest, PatchScore_404_MatchNotFound) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);

  EXPECT_CALL(mock, UpdateScore("NOPE", 2, 1)).WillOnce(Return(std::unexpected{404}));

  EXPECT_EQ(404, ctrl.PatchScore("NOPE", 2, 1));
}

TEST(MatchControllerTest, FinishMatch_204_Success) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);

  EXPECT_CALL(mock, FinishMatch("M1")).WillOnce(Return(std::expected<void,int>{}));

  EXPECT_EQ(204, ctrl.PostFinishMatch("M1"));
}

TEST(MatchControllerTest, FinishMatch_404_NotFound) {
  MockMatchDelegate mock;
  MatchController ctrl(&mock);

  EXPECT_CALL(mock, FinishMatch("NOPE")).WillOnce(Return(std::unexpected{404}));

  EXPECT_EQ(404, ctrl.PostFinishMatch("NOPE"));
}
