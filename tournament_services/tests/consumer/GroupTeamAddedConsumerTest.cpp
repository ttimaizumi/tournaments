#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "domain/Match.hpp"
#include "consumer/GroupTeamAddedConsumer.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

// Mock del GroupRepository
class MockGroupRepoForConsumer {
public:
    MOCK_METHOD(int, GroupSize, (const std::string& tid, const std::string& gid), ());
    MOCK_METHOD((std::vector<std::string>), GetTeamsInGroup,
                (const std::string& tid, const std::string& gid), ());
};

// Mock del MatchRepository
class MockMatchRepoForConsumer {
public:
    MOCK_METHOD((std::optional<std::string>), Insert, (const std::string& tid, const Match&), ());
};

//Tests de GroupTeamAddedConsumer - Formato Mundial

TEST(GroupTeamAddedConsumerTest, OnTeamAdded_LessThan4Teams_NoMatchesCreated) {
    // Con menos de 4 equipos, no se crean partidos
    MockGroupRepoForConsumer groupRepo;
    MockMatchRepoForConsumer matchRepo;
    GroupTeamAddedConsumer consumer(groupRepo, matchRepo);

    EXPECT_CALL(groupRepo, GroupSize("T1", "GA")).WillOnce(Return(3));
    // No debe llamar a GetTeamsInGroup ni Insert

    auto result = consumer.OnTeamAdded("T1", "GA", "TEAM3");
    ASSERT_TRUE(result);
    EXPECT_EQ(0, *result);  // 0 partidos creados
}

TEST(GroupTeamAddedConsumerTest, Mundial_FourthTeam_Creates6RoundRobinMatches) {
    // Al agregar el 4to equipo, se crean 6 partidos (round-robin)
    MockGroupRepoForConsumer groupRepo;
    MockMatchRepoForConsumer matchRepo;
    GroupTeamAddedConsumer consumer(groupRepo, matchRepo);

    EXPECT_CALL(groupRepo, GroupSize("T1", "GA")).WillOnce(Return(4));
    EXPECT_CALL(groupRepo, GetTeamsInGroup("T1", "GA"))
        .WillOnce(Return(std::vector<std::string>{"TEAM1", "TEAM2", "TEAM3", "TEAM4"}));

    // Debe llamar Insert exactamente 6 veces
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(6)
        .WillRepeatedly(Return(std::optional<std::string>{"M1"}));

    auto result = consumer.OnTeamAdded("T1", "GA", "TEAM4");
    ASSERT_TRUE(result);
    EXPECT_EQ(6, *result);  // 6 partidos creados
}

TEST(GroupTeamAddedConsumerTest, Mundial_AllMatchesHaveGroupPhase) {
    // Verifica que todos los partidos creados tengan phase="GROUP"
    MockGroupRepoForConsumer groupRepo;
    MockMatchRepoForConsumer matchRepo;
    GroupTeamAddedConsumer consumer(groupRepo, matchRepo);

    EXPECT_CALL(groupRepo, GroupSize("T1", "GA")).WillOnce(Return(4));
    EXPECT_CALL(groupRepo, GetTeamsInGroup("T1", "GA"))
        .WillOnce(Return(std::vector<std::string>{"T1", "T2", "T3", "T4"}));

    // Capturar los partidos para verificar sus campos
    int groupPhaseCount = 0;
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(6)
        .WillRepeatedly(Invoke([&groupPhaseCount](const std::string&, const Match& m) {
            if (m.phase == "GROUP") {
                groupPhaseCount++;
            }
            return std::optional<std::string>{"M1"};
        }));

    auto result = consumer.OnTeamAdded("T1", "GA", "T4");
    ASSERT_TRUE(result);
    EXPECT_EQ(6, groupPhaseCount);  // Los 6 partidos deben ser GROUP
}

TEST(GroupTeamAddedConsumerTest, Mundial_AllMatchesHaveCorrectGroupId) {
    // Verifica que todos los partidos tengan el groupId correcto
    MockGroupRepoForConsumer groupRepo;
    MockMatchRepoForConsumer matchRepo;
    GroupTeamAddedConsumer consumer(groupRepo, matchRepo);

    EXPECT_CALL(groupRepo, GroupSize("T1", "GB")).WillOnce(Return(4));
    EXPECT_CALL(groupRepo, GetTeamsInGroup("T1", "GB"))
        .WillOnce(Return(std::vector<std::string>{"T1", "T2", "T3", "T4"}));

    int correctGroupIdCount = 0;
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(6)
        .WillRepeatedly(Invoke([&correctGroupIdCount](const std::string&, const Match& m) {
            if (m.groupId == "GB") {
                correctGroupIdCount++;
            }
            return std::optional<std::string>{"M1"};
        }));

    auto result = consumer.OnTeamAdded("T1", "GB", "T4");
    ASSERT_TRUE(result);
    EXPECT_EQ(6, correctGroupIdCount);  // Los 6 partidos deben tener groupId="GB"
}

TEST(GroupTeamAddedConsumerTest, Mundial_RoundRobin_AllCombinations) {
    // Verifica que se crean todas las combinaciones posibles (C(4,2) = 6)
    MockGroupRepoForConsumer groupRepo;
    MockMatchRepoForConsumer matchRepo;
    GroupTeamAddedConsumer consumer(groupRepo, matchRepo);

    EXPECT_CALL(groupRepo, GroupSize("T1", "GA")).WillOnce(Return(4));
    EXPECT_CALL(groupRepo, GetTeamsInGroup("T1", "GA"))
        .WillOnce(Return(std::vector<std::string>{"BRASIL", "ALEMANIA", "FRANCIA", "ARGENTINA"}));

    // Capturar todas las combinaciones creadas
    std::vector<std::pair<std::string, std::string>> combinations;
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(6)
        .WillRepeatedly(Invoke([&combinations](const std::string&, const Match& m) {
            combinations.push_back({m.homeTeamId, m.awayTeamId});
            return std::optional<std::string>{"M1"};
        }));

    auto result = consumer.OnTeamAdded("T1", "GA", "ARGENTINA");
    ASSERT_TRUE(result);

    // Verificar que tenemos 6 combinaciones únicas
    EXPECT_EQ(6u, combinations.size());

    // Verificar combinaciones esperadas
    bool hasBrasilAlemania = false;
    bool hasBrasilFrancia = false;
    bool hasAlemaniaArgentina = false;

    for (const auto& combo : combinations) {
        if ((combo.first == "BRASIL" && combo.second == "ALEMANIA") ||
            (combo.first == "ALEMANIA" && combo.second == "BRASIL")) {
            hasBrasilAlemania = true;
        }
        if ((combo.first == "BRASIL" && combo.second == "FRANCIA") ||
            (combo.first == "FRANCIA" && combo.second == "BRASIL")) {
            hasBrasilFrancia = true;
        }
        if ((combo.first == "ALEMANIA" && combo.second == "ARGENTINA") ||
            (combo.first == "ARGENTINA" && combo.second == "ALEMANIA")) {
            hasAlemaniaArgentina = true;
        }
    }

    EXPECT_TRUE(hasBrasilAlemania);
    EXPECT_TRUE(hasBrasilFrancia);
    EXPECT_TRUE(hasAlemaniaArgentina);
}

TEST(GroupTeamAddedConsumerTest, OnTeamAdded_TeamListMismatch_Returns500) {
    // Si GetTeamsInGroup retorna cantidad diferente a 4, error 500
    MockGroupRepoForConsumer groupRepo;
    MockMatchRepoForConsumer matchRepo;
    GroupTeamAddedConsumer consumer(groupRepo, matchRepo);

    EXPECT_CALL(groupRepo, GroupSize("T1", "GA")).WillOnce(Return(4));
    EXPECT_CALL(groupRepo, GetTeamsInGroup("T1", "GA"))
        .WillOnce(Return(std::vector<std::string>{"T1", "T2"}));  // Solo 2 equipos

    auto result = consumer.OnTeamAdded("T1", "GA", "TEAM");
    ASSERT_FALSE(result);
    EXPECT_EQ(500, result.error());
}

TEST(GroupTeamAddedConsumerTest, OnTeamAdded_MatchInsertFails_Returns500) {
    // Si no se pueden crear todos los partidos, retorna error 500
    MockGroupRepoForConsumer groupRepo;
    MockMatchRepoForConsumer matchRepo;
    GroupTeamAddedConsumer consumer(groupRepo, matchRepo);

    EXPECT_CALL(groupRepo, GroupSize("T1", "GA")).WillOnce(Return(4));
    EXPECT_CALL(groupRepo, GetTeamsInGroup("T1", "GA"))
        .WillOnce(Return(std::vector<std::string>{"T1", "T2", "T3", "T4"}));

    // Solo 3 inserciones exitosas (falta 1)
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(6)
        .WillOnce(Return(std::optional<std::string>{"M1"}))
        .WillOnce(Return(std::optional<std::string>{"M2"}))
        .WillOnce(Return(std::optional<std::string>{"M3"}))
        .WillOnce(Return(std::nullopt))  // Falla
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));

    auto result = consumer.OnTeamAdded("T1", "GA", "T4");
    ASSERT_FALSE(result);
    EXPECT_EQ(500, result.error());  // No se crearon exactamente 6 partidos
}

TEST(GroupTeamAddedConsumerTest, Mundial_InitialScores_AreZero) {
    // Verifica que partidos creados tengan scores iniciales en 0
    MockGroupRepoForConsumer groupRepo;
    MockMatchRepoForConsumer matchRepo;
    GroupTeamAddedConsumer consumer(groupRepo, matchRepo);

    EXPECT_CALL(groupRepo, GroupSize("T1", "GA")).WillOnce(Return(4));
    EXPECT_CALL(groupRepo, GetTeamsInGroup("T1", "GA"))
        .WillOnce(Return(std::vector<std::string>{"T1", "T2", "T3", "T4"}));

    int zeroScoreCount = 0;
    EXPECT_CALL(matchRepo, Insert("T1", _))
        .Times(6)
        .WillRepeatedly(Invoke([&zeroScoreCount](const std::string&, const Match& m) {
            if (m.homeScore == 0 && m.awayScore == 0 && !m.isFinished) {
                zeroScoreCount++;
            }
            return std::optional<std::string>{"M1"};
        }));

    auto result = consumer.OnTeamAdded("T1", "GA", "T4");
    ASSERT_TRUE(result);
    EXPECT_EQ(6, zeroScoreCount);  // Los 6 partidos deben estar en 0-0 y no finalizados
}
