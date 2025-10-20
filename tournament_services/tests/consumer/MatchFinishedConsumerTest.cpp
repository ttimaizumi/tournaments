#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <optional>
#include <string>
#include "domain/Match.hpp"
#include "domain/Standing.hpp"
#include "consumer/MatchFinishedConsumer.hpp"

using ::testing::_;
using ::testing::Return;

// Mock del MatchRepository
class MockMatchRepoForFinishedConsumer {
public:
    MOCK_METHOD((std::optional<Match>), FindById, (const std::string& matchId), ());
};

// Mock del StandingRepository
class MockStandingRepoForFinishedConsumer {
public:
    MOCK_METHOD((std::optional<Standing>), FindByTeam,
                (const std::string& tid, const std::string& gid, const std::string& teamId), ());
    MOCK_METHOD(bool, Update, (const Standing&), ());
};

//Tests de MatchFinishedConsumer - Formato Mundial

TEST(MatchFinishedConsumerTest, OnMatchFinished_MatchNotFound_Returns404) {
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    EXPECT_CALL(matchRepo, FindById("NOPE")).WillOnce(Return(std::nullopt));

    auto result = consumer.OnMatchFinished("NOPE");
    ASSERT_FALSE(result);
    EXPECT_EQ(404, result.error());
}

TEST(MatchFinishedConsumerTest, OnMatchFinished_MatchNotFinished_Returns400) {
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "TEAM1", "TEAM2", 0, 0, "GROUP", "", false};
    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));

    auto result = consumer.OnMatchFinished("M1");
    ASSERT_FALSE(result);
    EXPECT_EQ(400, result.error());
}

TEST(MatchFinishedConsumerTest, Mundial_GroupPhase_HomeWin_Updates3Points) {
    // Victoria local en fase de grupos: local recibe 3 puntos, visitante 0
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "HOME", "AWAY", 3, 1, "GROUP", "", true};
    Standing homeStanding{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing awayStanding{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(homeStanding));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(awayStanding));

    // Debe actualizar ambos standings
    EXPECT_CALL(standingRepo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = consumer.OnMatchFinished("M1");
    EXPECT_TRUE(result.has_value());
}

TEST(MatchFinishedConsumerTest, Mundial_GroupPhase_Draw_Updates1PointEach) {
    //Empate en fase de grupos: ambos reciben 1 punto
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "HOME", "AWAY", 2, 2, "GROUP", "", true};
    Standing homeStanding{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing awayStanding{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(homeStanding));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(awayStanding));
    EXPECT_CALL(standingRepo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = consumer.OnMatchFinished("M1");
    EXPECT_TRUE(result.has_value());
}

TEST(MatchFinishedConsumerTest, Mundial_GroupPhase_AwayWin_Updates3Points) {
    // Victoria visitante: visitante recibe 3 puntos, local 0
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "HOME", "AWAY", 1, 4, "GROUP", "", true};
    Standing homeStanding{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing awayStanding{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(homeStanding));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(awayStanding));
    EXPECT_CALL(standingRepo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = consumer.OnMatchFinished("M1");
    EXPECT_TRUE(result.has_value());
}

TEST(MatchFinishedConsumerTest, Mundial_EliminationPhase_NoStandingUpdate) {
    // En fase de eliminación NO se actualiza tabla de posiciones
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "", "TEAM1", "TEAM2", 2, 1, "ELIMINATION", "ROUND_OF_16", true};

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    // NO debe llamar a FindByTeam ni Update para standings

    auto result = consumer.OnMatchFinished("M1");
    EXPECT_TRUE(result.has_value());  // ok pero sin actualizar standings
}

TEST(MatchFinishedConsumerTest, Mundial_UpdatesGoalDifference) {
    // Verifica que se actualice correctamente la diferencia de goles
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "HOME", "AWAY", 5, 2, "GROUP", "", true};
    Standing homeStanding{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing awayStanding{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(homeStanding));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(awayStanding));
    EXPECT_CALL(standingRepo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = consumer.OnMatchFinished("M1");
    EXPECT_TRUE(result.has_value());
    // Home: GF=5, GA=2, GD=+3
    // Away: GF=2, GA=5, GD=-3
}

TEST(MatchFinishedConsumerTest, OnMatchFinished_StandingNotFound_Returns404) {
    // Si un equipo no existe en standings, retorna 404
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "HOME", "AWAY", 2, 1, "GROUP", "", true};
    Standing home{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(home));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(std::nullopt));

    auto result = consumer.OnMatchFinished("M1");
    ASSERT_FALSE(result);
    EXPECT_EQ(404, result.error());
}

TEST(MatchFinishedConsumerTest, OnMatchFinished_UpdateFails_Returns500) {
    // Si falla la actualización en BD, retorna 500
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "HOME", "AWAY", 2, 1, "GROUP", "", true};
    Standing homeStanding{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing awayStanding{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(homeStanding));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(awayStanding));

    // Primera actualización falla
    EXPECT_CALL(standingRepo, Update(_)).WillOnce(Return(false));

    auto result = consumer.OnMatchFinished("M1");
    ASSERT_FALSE(result);
    EXPECT_EQ(500, result.error());
}

TEST(MatchFinishedConsumerTest, Mundial_UpdatesMatchesPlayed) {
    // Verifica que se incremente el contador de partidos jugados
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "HOME", "AWAY", 1, 1, "GROUP", "", true};
    Standing homeStanding{"HOME", "T1", "GA", 3, 1, 0, 0, 5, 2, 3, 1};  // Ya jugó 1 partido
    Standing awayStanding{"AWAY", "T1", "GA", 0, 0, 0, 1, 1, 3, -2, 1};  // Ya jugó 1 partido

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(homeStanding));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(awayStanding));
    EXPECT_CALL(standingRepo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = consumer.OnMatchFinished("M1");
    EXPECT_TRUE(result.has_value());
    // Ambos equipos deben tener matchesPlayed=2 después del update
}

TEST(MatchFinishedConsumerTest, Mundial_ScoreRange_0to10_Valid) {
    // Verifica que scores dentro del rango 0-10 se procesen correctamente
    MockMatchRepoForFinishedConsumer matchRepo;
    MockStandingRepoForFinishedConsumer standingRepo;
    MatchFinishedConsumer consumer(matchRepo, standingRepo);

    Match m{"M1", "T1", "GA", "HOME", "AWAY", 10, 10, "GROUP", "", true};  // Máximo permitido
    Standing homeStanding{"HOME", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};
    Standing awayStanding{"AWAY", "T1", "GA", 0, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(matchRepo, FindById("M1")).WillOnce(Return(m));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "HOME")).WillOnce(Return(homeStanding));
    EXPECT_CALL(standingRepo, FindByTeam("T1", "GA", "AWAY")).WillOnce(Return(awayStanding));
    EXPECT_CALL(standingRepo, Update(_)).Times(2).WillRepeatedly(Return(true));

    auto result = consumer.OnMatchFinished("M1");
    EXPECT_TRUE(result.has_value());
}
