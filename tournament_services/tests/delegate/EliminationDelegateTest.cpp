#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "domain/Standing.hpp"
#include "domain/Match.hpp"
#include "delegate/EliminationDelegate.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

// Mock del StandingRepository
class MockStandingRepoForElimination {
public:
    MOCK_METHOD((std::vector<Standing>), GetTopTeams,
                (const std::string& tid, const std::string& gid, int limit), ());
};

// Mock del MatchRepository
class MockMatchRepoForElimination {
public:
    MOCK_METHOD((std::optional<std::string>), Insert, (const std::string& tid, const Match&), ());
};

//Tests de EliminationDelegate - Cruces del Mundial

TEST(EliminationDelegateTest, GetMundialBrackets_Returns8Brackets) {
    // Verifica que el Mundial tenga exactamente 8 cruces de octavos
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    EXPECT_EQ(8u, brackets.size());
}

TEST(EliminationDelegateTest, Mundial_Bracket1_A1vsH2) {
    // Verifica que el primer cruce sea A1 vs H2
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    ASSERT_GE(brackets.size(), 1u);
    EXPECT_EQ("A1", brackets[0].first);
    EXPECT_EQ("H2", brackets[0].second);
}

TEST(EliminationDelegateTest, Mundial_Bracket2_B2vsG1) {
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    ASSERT_GE(brackets.size(), 2u);
    EXPECT_EQ("B2", brackets[1].first);
    EXPECT_EQ("G1", brackets[1].second);
}

TEST(EliminationDelegateTest, Mundial_Bracket3_C1vsF2) {
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    ASSERT_GE(brackets.size(), 3u);
    EXPECT_EQ("C1", brackets[2].first);
    EXPECT_EQ("F2", brackets[2].second);
}

TEST(EliminationDelegateTest, Mundial_Bracket4_D2vsE1) {
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    ASSERT_GE(brackets.size(), 4u);
    EXPECT_EQ("D2", brackets[3].first);
    EXPECT_EQ("E1", brackets[3].second);
}

TEST(EliminationDelegateTest, Mundial_Bracket5_D1vsE2) {
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    ASSERT_GE(brackets.size(), 5u);
    EXPECT_EQ("D1", brackets[4].first);
    EXPECT_EQ("E2", brackets[4].second);
}

TEST(EliminationDelegateTest, Mundial_Bracket6_C2vsF1) {
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    ASSERT_GE(brackets.size(), 6u);
    EXPECT_EQ("C2", brackets[5].first);
    EXPECT_EQ("F1", brackets[5].second);
}

TEST(EliminationDelegateTest, Mundial_Bracket7_B1vsG2) {
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    ASSERT_GE(brackets.size(), 7u);
    EXPECT_EQ("B1", brackets[6].first);
    EXPECT_EQ("G2", brackets[6].second);
}

TEST(EliminationDelegateTest, Mundial_Bracket8_A2vsH1) {
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    auto brackets = d.GetMundialBrackets();
    ASSERT_GE(brackets.size(), 8u);
    EXPECT_EQ("A2", brackets[7].first);
    EXPECT_EQ("H1", brackets[7].second);
}

TEST(EliminationDelegateTest, IsValidBracket_A1vsH2_True) {
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    EXPECT_TRUE(d.IsValidBracket("A1", "H2"));
}

TEST(EliminationDelegateTest, IsValidBracket_A1vsA2_False) {
    // A1 vs A2 no es un cruce válido del Mundial
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    EXPECT_FALSE(d.IsValidBracket("A1", "A2"));
}

TEST(EliminationDelegateTest, IsValidBracket_B1vsC1_False) {
    // B1 vs C1 no es un cruce válido del Mundial
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    EXPECT_FALSE(d.IsValidBracket("B1", "C1"));
}

TEST(EliminationDelegateTest, CreateRoundOf16_Creates8Matches) {
    // Al crear octavos, se deben crear exactamente 8 partidos
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    // Simular top 2 de cada grupo (A-H)
    for (char group = 'A'; group <= 'H'; group++) {
        std::string gid(1, group);
        std::string team1 = gid + "1";
        std::string team2 = gid + "2";

        std::vector<Standing> top2 = {
            Standing{team1, "T1", gid, 9, 3, 0, 0, 8, 2, 6, 3},
            Standing{team2, "T1", gid, 6, 2, 0, 1, 5, 3, 2, 3}
        };

        EXPECT_CALL(standingRepo, GetTopTeams("T1", gid, 2))
            .WillOnce(Return(top2));
    }

    // Debe crear 8 partidos
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(8)
        .WillRepeatedly(Return(std::optional<std::string>{"M1"}));

    auto result = d.CreateRoundOf16("T1");
    ASSERT_TRUE(result);
    EXPECT_EQ(8, *result);
}

TEST(EliminationDelegateTest, CreateRoundOf16_AllMatchesAreEliminationPhase) {
    // Todos los partidos de octavos deben tener phase="ELIMINATION"
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    // Setup standings
    for (char group = 'A'; group <= 'H'; group++) {
        std::string gid(1, group);
        std::vector<Standing> top2 = {
            Standing{gid + "1", "T1", gid, 9, 3, 0, 0, 8, 2, 6, 3},
            Standing{gid + "2", "T1", gid, 6, 2, 0, 1, 5, 3, 2, 3}
        };
        EXPECT_CALL(standingRepo, GetTopTeams("T1", gid, 2)).WillOnce(Return(top2));
    }

    int eliminationCount = 0;
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(8)
        .WillRepeatedly(Invoke([&eliminationCount](const std::string&, const Match& m) {
            if (m.phase == "ELIMINATION" && m.round == "ROUND_OF_16") {
                eliminationCount++;
            }
            return std::optional<std::string>{"M1"};
        }));

    auto result = d.CreateRoundOf16("T1");
    ASSERT_TRUE(result);
    EXPECT_EQ(8, eliminationCount);
}

TEST(EliminationDelegateTest, CreateRoundOf16_GroupWithoutTop2_Returns400) {
    // Si un grupo no tiene 2 equipos clasificados, error 400
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    // Grupo A tiene 2 equipos
    std::vector<Standing> topA = {
        Standing{"A1", "T1", "A", 9, 3, 0, 0, 8, 2, 6, 3},
        Standing{"A2", "T1", "A", 6, 2, 0, 1, 5, 3, 2, 3}
    };
    EXPECT_CALL(standingRepo, GetTopTeams("T1", "A", 2)).WillOnce(Return(topA));

    // Grupo B solo tiene 1 equipo (error)
    std::vector<Standing> topB = {
        Standing{"B1", "T1", "B", 9, 3, 0, 0, 8, 2, 6, 3}
    };
    EXPECT_CALL(standingRepo, GetTopTeams("T1", "B", 2)).WillOnce(Return(topB));

    auto result = d.CreateRoundOf16("T1");
    ASSERT_FALSE(result);
    EXPECT_EQ(400, result.error());
}

TEST(EliminationDelegateTest, CreateRoundOf16_MatchInsertFails_Returns500) {
    // Si no se pueden crear todos los partidos, error 500
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    // Setup standings
    for (char group = 'A'; group <= 'H'; group++) {
        std::string gid(1, group);
        std::vector<Standing> top2 = {
            Standing{gid + "1", "T1", gid, 9, 3, 0, 0, 8, 2, 6, 3},
            Standing{gid + "2", "T1", gid, 6, 2, 0, 1, 5, 3, 2, 3}
        };
        EXPECT_CALL(standingRepo, GetTopTeams("T1", gid, 2)).WillOnce(Return(top2));
    }

    // Solo 5 inserciones exitosas (faltan 3)
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(8)
        .WillOnce(Return(std::optional<std::string>{"M1"}))
        .WillOnce(Return(std::optional<std::string>{"M2"}))
        .WillOnce(Return(std::optional<std::string>{"M3"}))
        .WillOnce(Return(std::optional<std::string>{"M4"}))
        .WillOnce(Return(std::optional<std::string>{"M5"}))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));

    auto result = d.CreateRoundOf16("T1");
    ASSERT_FALSE(result);
    EXPECT_EQ(500, result.error());
}

TEST(EliminationDelegateTest, Mundial_VerifyAllCruces_CorrectPairings) {
    // Verifica que todos los cruces del Mundial sean correctos
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    // Verificar todos los cruces válidos
    EXPECT_TRUE(d.IsValidBracket("A1", "H2"));
    EXPECT_TRUE(d.IsValidBracket("B2", "G1"));
    EXPECT_TRUE(d.IsValidBracket("C1", "F2"));
    EXPECT_TRUE(d.IsValidBracket("D2", "E1"));
    EXPECT_TRUE(d.IsValidBracket("D1", "E2"));
    EXPECT_TRUE(d.IsValidBracket("C2", "F1"));
    EXPECT_TRUE(d.IsValidBracket("B1", "G2"));
    EXPECT_TRUE(d.IsValidBracket("A2", "H1"));
}

TEST(EliminationDelegateTest, CreateRoundOf16_NoGroupId_InEliminationMatches) {
    // Los partidos de eliminación no deben tener groupId
    MockStandingRepoForElimination standingRepo;
    MockMatchRepoForElimination matchRepo;
    EliminationDelegate d(standingRepo, matchRepo);

    for (char group = 'A'; group <= 'H'; group++) {
        std::string gid(1, group);
        std::vector<Standing> top2 = {
            Standing{gid + "1", "T1", gid, 9, 3, 0, 0, 8, 2, 6, 3},
            Standing{gid + "2", "T1", gid, 6, 2, 0, 1, 5, 3, 2, 3}
        };
        EXPECT_CALL(standingRepo, GetTopTeams("T1", gid, 2)).WillOnce(Return(top2));
    }

    int emptyGroupIdCount = 0;
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(8)
        .WillRepeatedly(Invoke([&emptyGroupIdCount](const std::string&, const Match& m) {
            if (m.groupId.empty()) {
                emptyGroupIdCount++;
            }
            return std::optional<std::string>{"M1"};
        }));

    auto result = d.CreateRoundOf16("T1");
    ASSERT_TRUE(result);
    EXPECT_EQ(8, emptyGroupIdCount);  // Todos sin groupId
}
