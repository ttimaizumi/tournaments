#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "domain/Standing.hpp"
#include "delegate/StandingDelegate.hpp"

using ::testing::_;
using ::testing::Return;

// Mock del StandingRepository
class MockStandingRepository {
public:
    MOCK_METHOD((std::optional<Standing>), FindByTeam,
                (const std::string& tid, const std::string& gid, const std::string& teamId), ());
    MOCK_METHOD((std::vector<Standing>), ListByGroup,
                (const std::string& tid, const std::string& gid), ());
    MOCK_METHOD(bool, Update, (const Standing&), ());
};

//Tests de StandingDelegate - Formato Mundial

TEST(StandingDelegateTest, GetStanding_Found) {
    MockStandingRepository repo;
    StandingDelegate d(repo);

    Standing expected{"TEAM1", "T1", "GA", 6, 2, 0, 0, 5, 2, 3, 2};
    EXPECT_CALL(repo, FindByTeam("T1", "GA", "TEAM1")).WillOnce(Return(expected));

    auto result = d.GetStanding("T1", "GA", "TEAM1");
    ASSERT_TRUE(result);
    EXPECT_EQ(6, result->points);
    EXPECT_EQ(2, result->wins);
}

TEST(StandingDelegateTest, GetStanding_NotFound) {
    MockStandingRepository repo;
    StandingDelegate d(repo);

    EXPECT_CALL(repo, FindByTeam("T1", "GA", "NOPE")).WillOnce(Return(std::nullopt));

    auto result = d.GetStanding("T1", "GA", "NOPE");
    EXPECT_FALSE(result.has_value());
}

TEST(StandingDelegateTest, ListStandings_ReturnsAll) {
    MockStandingRepository repo;
    StandingDelegate d(repo);

    std::vector<Standing> standings = {
        Standing{"TEAM1", "T1", "GA", 6, 2, 0, 0, 5, 2, 3, 2},
        Standing{"TEAM2", "T1", "GA", 3, 1, 0, 1, 3, 3, 0, 2}
    };

    EXPECT_CALL(repo, ListByGroup("T1", "GA")).WillOnce(Return(standings));

    auto result = d.ListStandings("T1", "GA");
    EXPECT_EQ(2u, result.size());
}

// ---- Tests de clasificación según reglas del Mundial ----

TEST(StandingDelegateTest, Mundial_GetTop2Teams_OrderedByPoints) {
    // Verifica que los equipos se ordenen por puntos (criterio 1)
    MockStandingRepository repo;
    StandingDelegate d(repo);

    std::vector<Standing> standings = {
        Standing{"TEAM3", "T1", "GA", 3, 1, 0, 1, 3, 4, -1, 2},  // 3 puntos
        Standing{"TEAM1", "T1", "GA", 9, 3, 0, 0, 8, 2, 6, 3},   // 9 puntos - 1ro
        Standing{"TEAM4", "T1", "GA", 0, 0, 0, 3, 1, 7, -6, 3},  // 0 puntos
        Standing{"TEAM2", "T1", "GA", 6, 2, 0, 1, 5, 3, 2, 3}    // 6 puntos - 2do
    };

    EXPECT_CALL(repo, ListByGroup("T1", "GA")).WillOnce(Return(standings));

    auto top2 = d.GetTopTeams("T1", "GA", 2);
    ASSERT_EQ(2u, top2.size());
    EXPECT_EQ("TEAM1", top2[0].teamId);  // 9 puntos
    EXPECT_EQ("TEAM2", top2[1].teamId);  // 6 puntos
}

TEST(StandingDelegateTest, Mundial_GetTop2Teams_SamePoints_OrderedByGoalDifference) {
    // Verifica que con mismos puntos, se ordenen por diferencia de goles (criterio 2)
    MockStandingRepository repo;
    StandingDelegate d(repo);

    std::vector<Standing> standings = {
        Standing{"TEAM1", "T1", "GA", 6, 2, 0, 1, 5, 3, 2, 3},   // 6 pts, GD +2
        Standing{"TEAM2", "T1", "GA", 6, 2, 0, 1, 7, 3, 4, 3},   // 6 pts, GD +4 - 1ro
        Standing{"TEAM3", "T1", "GA", 6, 2, 0, 1, 4, 3, 1, 3},   // 6 pts, GD +1
        Standing{"TEAM4", "T1", "GA", 0, 0, 0, 3, 2, 8, -6, 3}   // 0 pts
    };

    EXPECT_CALL(repo, ListByGroup("T1", "GA")).WillOnce(Return(standings));

    auto top2 = d.GetTopTeams("T1", "GA", 2);
    ASSERT_EQ(2u, top2.size());
    EXPECT_EQ("TEAM2", top2[0].teamId);  // GD +4
    EXPECT_EQ("TEAM1", top2[1].teamId);  // GD +2
}

TEST(StandingDelegateTest, Mundial_GetTop2Teams_SamePoints_SameGD_OrderedByGoalsFor) {
    // Verifica que con mismos puntos y GD, se ordenen por goles a favor (criterio 3)
    MockStandingRepository repo;
    StandingDelegate d(repo);

    std::vector<Standing> standings = {
        Standing{"TEAM1", "T1", "GA", 4, 1, 1, 1, 4, 3, 1, 3},   // 4 pts, GD +1, GF 4
        Standing{"TEAM2", "T1", "GA", 4, 1, 1, 1, 5, 4, 1, 3},   // 4 pts, GD +1, GF 5 - 1ro
        Standing{"TEAM3", "T1", "GA", 4, 1, 1, 1, 3, 2, 1, 3},   // 4 pts, GD +1, GF 3
        Standing{"TEAM4", "T1", "GA", 0, 0, 0, 3, 1, 7, -6, 3}   // 0 pts
    };

    EXPECT_CALL(repo, ListByGroup("T1", "GA")).WillOnce(Return(standings));

    auto top2 = d.GetTopTeams("T1", "GA", 2);
    ASSERT_EQ(2u, top2.size());
    EXPECT_EQ("TEAM2", top2[0].teamId);  // GF 5
    EXPECT_EQ("TEAM1", top2[1].teamId);  // GF 4
}

// ---- Tests de actualización de standings después de partido ----

TEST(StandingDelegateTest, UpdateAfterMatch_HomeWin_3Points) {
    // Victoria local: 3 puntos para local, 0 para visitante
    MockStandingRepository repo;
    StandingDelegate d(repo);

    Standing home{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing away{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(repo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(home));
    EXPECT_CALL(repo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(away));

    // Esperar que se actualicen ambos standings
    EXPECT_CALL(repo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = d.UpdateAfterMatch("T1", "GA", "HOME", "AWAY", 3, 1);
    EXPECT_TRUE(result.has_value());
}

TEST(StandingDelegateTest, UpdateAfterMatch_Draw_1PointEach) {
    // Empate: 1 punto para cada equipo
    MockStandingRepository repo;
    StandingDelegate d(repo);

    Standing home{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing away{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(repo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(home));
    EXPECT_CALL(repo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(away));
    EXPECT_CALL(repo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = d.UpdateAfterMatch("T1", "GA", "HOME", "AWAY", 2, 2);
    EXPECT_TRUE(result.has_value());
}

TEST(StandingDelegateTest, UpdateAfterMatch_AwayWin_3Points) {
    // Victoria visitante: 0 puntos para local, 3 para visitante
    MockStandingRepository repo;
    StandingDelegate d(repo);

    Standing home{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing away{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(repo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(home));
    EXPECT_CALL(repo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(away));
    EXPECT_CALL(repo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = d.UpdateAfterMatch("T1", "GA", "HOME", "AWAY", 0, 2);
    EXPECT_TRUE(result.has_value());
}

TEST(StandingDelegateTest, UpdateAfterMatch_NegativeScore_Returns400) {
    // Marcador negativo retorna error 400
    MockStandingRepository repo;
    StandingDelegate d(repo);

    auto result = d.UpdateAfterMatch("T1", "GA", "HOME", "AWAY", -1, 2);
    ASSERT_FALSE(result);
    EXPECT_EQ(400, result.error());
}

TEST(StandingDelegateTest, UpdateAfterMatch_TeamNotFound_Returns404) {
    // Si un equipo no existe en standings, retorna 404
    MockStandingRepository repo;
    StandingDelegate d(repo);

    Standing home{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_CALL(repo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(home));
    EXPECT_CALL(repo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(std::nullopt));

    auto result = d.UpdateAfterMatch("T1", "GA", "HOME", "AWAY", 2, 1);
    ASSERT_FALSE(result);
    EXPECT_EQ(404, result.error());
}
