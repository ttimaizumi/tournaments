#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "domain/Match.hpp"
#include "delegate/MatchDelegate.hpp"

using ::testing::_;
using ::testing::Return;

// === Mocks específicos para MatchDelegate ===
class MockMatchRepositoryForDelegate {
public:
  MOCK_METHOD((std::optional<std::string>), Insert, (const std::string& tid, const Match&), ());
  MOCK_METHOD((std::optional<Match>),       FindById, (const std::string&), ());
  MOCK_METHOD((std::vector<Match>),         ListByGroup, (const std::string& tid, const std::string& gid), ());
  MOCK_METHOD(bool,                         UpdateScore, (const std::string&, int, int), ());
  MOCK_METHOD(bool,                         FinishMatch, (const std::string&), ());
};

class MockEventBusForMatchDelegate {
public:
  MOCK_METHOD(void, Publish, (const std::string& topic, const std::string& payload), ());
};

//Tests de MatchDelegate - Validaciones del Mundial

TEST(MatchDelegateTest, CreateMatch_Ok_PublishesEvent) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match m{"", "T1", "GA", "TEAM1", "TEAM2"};
  m.phase = "GROUP";

  EXPECT_CALL(repo, Insert("T1", _)).WillOnce(Return(std::optional<std::string>{"M1"}));
  EXPECT_CALL(bus, Publish("match-created", "M1"));

  auto r = d.CreateMatch("T1", m);
  ASSERT_TRUE(r);
  EXPECT_EQ("M1", *r);
}

TEST(MatchDelegateTest, CreateMatch_Conflict409) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match m{"", "T1", "GA", "TEAM1", "TEAM2"};
  EXPECT_CALL(repo, Insert("T1", _)).WillOnce(Return(std::nullopt));

  auto r = d.CreateMatch("T1", m);
  ASSERT_FALSE(r);
  EXPECT_EQ(409, r.error());
}

TEST(MatchDelegateTest, FindMatchById_Found) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match expected{"M1", "T1", "GA", "TEAM1", "TEAM2"};
  EXPECT_CALL(repo, FindById("M1")).WillOnce(Return(expected));

  auto r = d.FindMatchById("M1");
  ASSERT_TRUE(r);
  EXPECT_EQ("TEAM1", r->homeTeamId);
}

TEST(MatchDelegateTest, FindMatchById_Null) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  EXPECT_CALL(repo, FindById("NOPE")).WillOnce(Return(std::nullopt));

  auto r = d.FindMatchById("NOPE");
  EXPECT_FALSE(r.has_value());
}

TEST(MatchDelegateTest, ListMatchesByGroup_ReturnsMatches) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  std::vector<Match> matches = {
    Match{"M1", "T1", "GA", "T1", "T2"},
    Match{"M2", "T1", "GA", "T3", "T4"}
  };

  EXPECT_CALL(repo, ListByGroup("T1", "GA")).WillOnce(Return(matches));

  auto v = d.ListMatchesByGroup("T1", "GA");
  EXPECT_EQ(2u, v.size());
}

// ---- VALIDACIONES DEL MUNDIAL ----

TEST(MatchDelegateTest, UpdateScore_ValidRange_0to10_Success) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match m{"M1", "T1", "GA", "T1", "T2"};
  m.phase = "GROUP";

  EXPECT_CALL(repo, FindById("M1")).WillOnce(Return(m));
  EXPECT_CALL(repo, UpdateScore("M1", 3, 2)).WillOnce(Return(true));
  EXPECT_CALL(bus, Publish("score-updated", "M1"));

  auto r = d.UpdateScore("M1", 3, 2);
  EXPECT_TRUE(r.has_value());
}

TEST(MatchDelegateTest, UpdateScore_MaxValidScore_10) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match m{"M1", "T1", "GA", "T1", "T2"};
  m.phase = "GROUP";

  EXPECT_CALL(repo, FindById("M1")).WillOnce(Return(m));
  EXPECT_CALL(repo, UpdateScore("M1", 10, 10)).WillOnce(Return(true));
  EXPECT_CALL(bus, Publish("score-updated", "M1"));

  auto r = d.UpdateScore("M1", 10, 10);
  EXPECT_TRUE(r.has_value());
}

TEST(MatchDelegateTest, UpdateScore_Above10_Returns400) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  // NO debe llamar a FindById porque la validación es antes
  auto r = d.UpdateScore("M1", 11, 5);
  ASSERT_FALSE(r);
  EXPECT_EQ(400, r.error());
}

TEST(MatchDelegateTest, UpdateScore_Negative_Returns400) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  auto r = d.UpdateScore("M1", -1, 2);
  ASSERT_FALSE(r);
  EXPECT_EQ(400, r.error());
}

TEST(MatchDelegateTest, UpdateScore_DrawInGroupPhase_Allowed) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match m{"M1", "T1", "GA", "T1", "T2"};
  m.phase = "GROUP";

  EXPECT_CALL(repo, FindById("M1")).WillOnce(Return(m));
  EXPECT_CALL(repo, UpdateScore("M1", 2, 2)).WillOnce(Return(true));
  EXPECT_CALL(bus, Publish("score-updated", "M1"));

  auto r = d.UpdateScore("M1", 2, 2);
  EXPECT_TRUE(r.has_value());
}

TEST(MatchDelegateTest, UpdateScore_DrawInEliminationPhase_Returns422) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match m{"M1", "T1", "", "T1", "T2"};
  m.phase = "ELIMINATION";

  EXPECT_CALL(repo, FindById("M1")).WillOnce(Return(m));

  auto r = d.UpdateScore("M1", 2, 2);
  ASSERT_FALSE(r);
  EXPECT_EQ(422, r.error());
}

TEST(MatchDelegateTest, UpdateScore_MatchNotFound_Returns404) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  EXPECT_CALL(repo, FindById("NOPE")).WillOnce(Return(std::nullopt));

  auto r = d.UpdateScore("NOPE", 2, 1);
  ASSERT_FALSE(r);
  EXPECT_EQ(404, r.error());
}

TEST(MatchDelegateTest, FinishMatch_Success_PublishesEvent) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match m{"M1", "T1", "GA", "T1", "T2"};
  EXPECT_CALL(repo, FindById("M1")).WillOnce(Return(m));
  EXPECT_CALL(repo, FinishMatch("M1")).WillOnce(Return(true));
  EXPECT_CALL(bus, Publish("match-finished", "M1"));

  auto r = d.FinishMatch("M1");
  EXPECT_TRUE(r.has_value());
}

TEST(MatchDelegateTest, FinishMatch_NotFound_Returns404) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  EXPECT_CALL(repo, FindById("NOPE")).WillOnce(Return(std::nullopt));

  auto r = d.FinishMatch("NOPE");
  ASSERT_FALSE(r);
  EXPECT_EQ(404, r.error());
}

// ---- Test de validación de equipos del mismo grupo (fase de grupos) ----

TEST(MatchDelegateTest, CreateMatch_GroupPhase_EmptyTeam_Returns400) {
  MockMatchRepositoryForDelegate repo;
  MockEventBusForMatchDelegate bus;
  MatchDelegate d(repo, bus);

  Match m{"", "T1", "GA", "", "TEAM2"};
  m.phase = "GROUP";

  auto r = d.CreateMatch("T1", m);
  ASSERT_FALSE(r);
  EXPECT_EQ(400, r.error());
}

// ---- Tests de validación Mundial: Equipos del mismo grupo ----

// Mock del TeamRepository para validar grupos
class MockTeamRepoForMatchDelegate {
public:
    MOCK_METHOD((std::optional<std::string>), GetTeamGroup,
                (const std::string& tid, const std::string& teamId), ());
};

TEST(MatchDelegateTest, Mundial_GroupPhase_TeamsSameGroup_Ok) {
    // En fase de grupos, ambos equipos deben pertenecer al mismo grupo
    MockMatchRepositoryForDelegate repo;
    MockEventBusForMatchDelegate bus;
    MockTeamRepoForMatchDelegate teamRepo;
    MatchDelegate d(repo, bus, teamRepo);

    Match m{"", "T1", "GA", "BRASIL", "ALEMANIA"};
    m.phase = "GROUP";

    // Ambos equipos pertenecen al grupo A
    EXPECT_CALL(teamRepo, GetTeamGroup("T1", "BRASIL")).WillOnce(Return(std::optional<std::string>{"GA"}));
    EXPECT_CALL(teamRepo, GetTeamGroup("T1", "ALEMANIA")).WillOnce(Return(std::optional<std::string>{"GA"}));
    EXPECT_CALL(repo, Insert("T1", _)).WillOnce(Return(std::optional<std::string>{"M1"}));
    EXPECT_CALL(bus, Publish("match-created", "M1"));

    auto r = d.CreateMatch("T1", m);
    ASSERT_TRUE(r);
    EXPECT_EQ("M1", *r);
}

TEST(MatchDelegateTest, Mundial_GroupPhase_TeamsDifferentGroups_Returns422) {
    // En fase de grupos, NO se puede crear partido entre equipos de grupos diferentes
    MockMatchRepositoryForDelegate repo;
    MockEventBusForMatchDelegate bus;
    MockTeamRepoForMatchDelegate teamRepo;
    MatchDelegate d(repo, bus, teamRepo);

    Match m{"", "T1", "GA", "BRASIL", "ARGENTINA"};
    m.phase = "GROUP";

    // BRASIL está en grupo A, ARGENTINA en grupo B -> Error 422
    EXPECT_CALL(teamRepo, GetTeamGroup("T1", "BRASIL")).WillOnce(Return(std::optional<std::string>{"GA"}));
    EXPECT_CALL(teamRepo, GetTeamGroup("T1", "ARGENTINA")).WillOnce(Return(std::optional<std::string>{"GB"}));

    auto r = d.CreateMatch("T1", m);
    ASSERT_FALSE(r);
    EXPECT_EQ(422, r.error());
}

TEST(MatchDelegateTest, Mundial_GroupPhase_TeamNotInGroup_Returns404) {
    // Si un equipo no está asignado a ningún grupo, error 404
    MockMatchRepositoryForDelegate repo;
    MockEventBusForMatchDelegate bus;
    MockTeamRepoForMatchDelegate teamRepo;
    MatchDelegate d(repo, bus, teamRepo);

    Match m{"", "T1", "GA", "BRASIL", "FANTASMA"};
    m.phase = "GROUP";

    EXPECT_CALL(teamRepo, GetTeamGroup("T1", "BRASIL")).WillOnce(Return(std::optional<std::string>{"GA"}));
    EXPECT_CALL(teamRepo, GetTeamGroup("T1", "FANTASMA")).WillOnce(Return(std::nullopt));

    auto r = d.CreateMatch("T1", m);
    ASSERT_FALSE(r);
    EXPECT_EQ(404, r.error());
}

TEST(MatchDelegateTest, Mundial_GroupPhase_GroupMismatch_Returns422) {
    // Si el groupId del match no coincide con el grupo real de los equipos, error 422
    MockMatchRepositoryForDelegate repo;
    MockEventBusForMatchDelegate bus;
    MockTeamRepoForMatchDelegate teamRepo;
    MatchDelegate d(repo, bus, teamRepo);

    Match m{"", "T1", "GB", "BRASIL", "ALEMANIA"};  // Dice grupo B
    m.phase = "GROUP";

    // Pero ambos equipos están en grupo A
    EXPECT_CALL(teamRepo, GetTeamGroup("T1", "BRASIL")).WillOnce(Return(std::optional<std::string>{"GA"}));
    EXPECT_CALL(teamRepo, GetTeamGroup("T1", "ALEMANIA")).WillOnce(Return(std::optional<std::string>{"GA"}));

    auto r = d.CreateMatch("T1", m);
    ASSERT_FALSE(r);
    EXPECT_EQ(422, r.error());
}

TEST(MatchDelegateTest, Mundial_EliminationPhase_DifferentGroups_Ok) {
    // En fase de eliminación, los equipos PUEDEN ser de grupos diferentes
    MockMatchRepositoryForDelegate repo;
    MockEventBusForMatchDelegate bus;
    MatchDelegate d(repo, bus);  // Sin TeamRepo, no valida grupos

    Match m{"", "T1", "", "BRASIL", "ARGENTINA"};
    m.phase = "ELIMINATION";
    m.round = "ROUND_OF_16";

    EXPECT_CALL(repo, Insert("T1", _)).WillOnce(Return(std::optional<std::string>{"M1"}));
    EXPECT_CALL(bus, Publish("match-created", "M1"));

    auto r = d.CreateMatch("T1", m);
    ASSERT_TRUE(r);
    EXPECT_EQ("M1", *r);
}
