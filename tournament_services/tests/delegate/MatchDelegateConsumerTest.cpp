#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"

#include "../../tournament_consumer/include/delegate/MatchConsumerDelegate.hpp"

#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"
#include "domain/Team.hpp"

#include "mocks/MatchRepositoryMock.hpp"
#include "mocks/GroupRepositoryMock.hpp"
#include "mocks/TournamentRepositoryMock.h"

using ::testing::NiceMock;
using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;

// ------------------------------
// Helpers
// ------------------------------
namespace {

std::shared_ptr<domain::Group> mkGroup(
    std::string id,
    std::string name,
    std::vector<std::string> teams
) {
    auto g = std::make_shared<domain::Group>(name, id);
    for (auto& t : teams) {
        domain::Team team;
        team.Id = t;
        team.Name = t;
        g->Teams().push_back(team);
    }
    return g;
}

struct Fixture {
    NiceMock<MatchRepositoryMock>      matchRepoMock;
    NiceMock<GroupRepositoryMock>      groupRepoMock;
    NiceMock<TournamentRepositoryMock> tournamentRepoMock;

    std::shared_ptr<IMatchRepository> matchRepo{
        &matchRepoMock, [](IMatchRepository*){}
    };
    std::shared_ptr<IGroupRepository> groupRepo{
        &groupRepoMock, [](IGroupRepository*){}
    };
    std::shared_ptr<TournamentRepository> tournamentRepo{
        &tournamentRepoMock, [](TournamentRepository*){}
    };

    MatchConsumerDelegate delegate{matchRepo, groupRepo, tournamentRepo};
};

} // namespace

// ============================================================================
// 1) TEAM ADDITION -> TORNEO NO LISTO
// ============================================================================
TEST(MatchConsumerDelegateTest,
     ProcessTeamAddition_TournamentNotReady_LogsAndDoesNotCreateMatches) {
    Fixture fx;

    TeamAddEvent evt{"TID-NOT-READY", "G1", "TEAM-1"};

    auto tournament = std::make_shared<domain::Tournament>(
        "World Cup", domain::TournamentFormat{2, 3});
    tournament->Id() = evt.tournamentId;

    auto g1 = mkGroup("G1", "Group 1", {"A1", "A2", "A3"});
    auto g2 = mkGroup("G2", "Group 2", {"B1", "B2"}); // incompleto
    std::vector<std::shared_ptr<domain::Group>> groups{g1, g2};

    EXPECT_CALL(fx.tournamentRepoMock, ReadById(evt.tournamentId))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(tournament));

    EXPECT_CALL(fx.groupRepoMock,
                FindByTournamentIdAndGroupId(evt.tournamentId, evt.groupId))
        .Times(1)
        .WillOnce(Return(g1));

    EXPECT_CALL(fx.groupRepoMock,
                FindByTournamentId(evt.tournamentId))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(groups));

    EXPECT_CALL(fx.matchRepoMock, Create(_))
        .Times(0);

    testing::internal::CaptureStdout();
    fx.delegate.ProcessTeamAddition(evt);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("[MatchDelegate/WC] Team added in tournament: "
                       + evt.tournamentId),
              std::string::npos);

    EXPECT_NE(out.find("Tournament not ready yet; waiting for more teams"),
              std::string::npos);
}

// ============================================================================
// 2) TEAM ADDITION -> TORNEO LISTO
// ============================================================================
TEST(MatchConsumerDelegateTest,
     ProcessTeamAddition_TournamentReady_LogsAndTriggersCreation) {
    Fixture fx;

    TeamAddEvent evt{"TID-READY", "G1", "X-Team"};

    auto tournament = std::make_shared<domain::Tournament>(
        "World Cup", domain::TournamentFormat{2, 3});
    tournament->Id() = evt.tournamentId;

    // ambos completos
    auto g1 = mkGroup("G1", "Group 1", {"A1", "A2", "A3"});
    auto g2 = mkGroup("G2", "Group 2", {"B1", "B2", "B3"});
    std::vector<std::shared_ptr<domain::Group>> groups{g1, g2};

    EXPECT_CALL(fx.tournamentRepoMock, ReadById(evt.tournamentId))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(tournament));

    EXPECT_CALL(fx.groupRepoMock,
                FindByTournamentIdAndGroupId(evt.tournamentId, evt.groupId))
        .Times(1)
        .WillOnce(Return(g1));

    EXPECT_CALL(fx.groupRepoMock,
                FindByTournamentId(evt.tournamentId))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(groups));

    EXPECT_CALL(fx.matchRepoMock, Create(_))
        .Times(AtLeast(1));

    testing::internal::CaptureStdout();
    fx.delegate.ProcessTeamAddition(evt);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("[MatchDelegate/WC] Team added in tournament: "
                       + evt.tournamentId),
              std::string::npos);

    EXPECT_NE(out.find("All groups complete -> creating group-stage matches"),
              std::string::npos);
}

// ============================================================================
// 3) SCORE UPDATE -> SIGUEN PARTIDOS DE GRUPOS PENDIENTES
// ============================================================================
TEST(MatchConsumerDelegateTest,
     ProcessScoreUpdate_PendingMatches_ShowsPendingAndStops) {
    Fixture fx;

    ScoreUpdateEvent evt{"TID-PENDING"};

    auto m = std::make_shared<domain::Match>();
    m->TournamentId() = evt.tournamentId;
    m->Round()        = "group"; // sin score => pendiente

    std::vector<std::shared_ptr<domain::Match>> matches{m};

    EXPECT_CALL(fx.matchRepoMock, FindByTournamentId(evt.tournamentId))
        .Times(1)
        .WillOnce(Return(matches));

    EXPECT_CALL(fx.tournamentRepoMock, ReadById(_)).Times(0);
    EXPECT_CALL(fx.groupRepoMock, FindByTournamentId(_)).Times(0);

    testing::internal::CaptureStdout();
    fx.delegate.ProcessScoreUpdate(evt);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("Score update for tournament: " + evt.tournamentId),
              std::string::npos);

    EXPECT_NE(out.find("Still pending group matches"),
              std::string::npos);
}

// ============================================================================
// 4) SCORE UPDATE -> TODOS LOS PARTIDOS DE GRUPOS JUGADOS, PERO TORNEO NO EXISTE
// ============================================================================
TEST(MatchConsumerDelegateTest,
     ProcessScoreUpdate_AllPlayed_TournamentMissing_ShowsError) {
    Fixture fx;

    ScoreUpdateEvent evt{"TID-NOT-FOUND"};

    std::vector<std::shared_ptr<domain::Match>> matches; // vacíos = jugados todos

    EXPECT_CALL(fx.matchRepoMock, FindByTournamentId(evt.tournamentId))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(matches));

    EXPECT_CALL(fx.tournamentRepoMock, ReadById(evt.tournamentId))
        .Times(1)
        .WillOnce(Return(nullptr));

    EXPECT_CALL(fx.groupRepoMock, FindByTournamentId(_)).Times(0);

    testing::internal::CaptureStdout();
    fx.delegate.ProcessScoreUpdate(evt);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("Score update for tournament: " + evt.tournamentId),
              std::string::npos);

    EXPECT_NE(out.find("[WC] ERROR: tournament not found"),
              std::string::npos);
}



// ============================================================================
// 5) SCORE UPDATE -> SCORE UPDATE PERO NO ES FASE DE GRUPOS (NO BLOQUEA)
// ============================================================================
TEST(MatchConsumerDelegateTest,
     ProcessScoreUpdate_NonGroupMatches_DoesNotBlock) {
    Fixture fx;

    ScoreUpdateEvent evt{"TID-NONGROUP"};

    auto match = std::make_shared<domain::Match>();
    match->TournamentId() = evt.tournamentId;
    match->Round()        = "qf";
    match->Home().Id()    = "X1";
    match->Visitor().Id() = "X2";

    std::vector<std::shared_ptr<domain::Match>> matches{match};

    EXPECT_CALL(fx.matchRepoMock, FindByTournamentId(evt.tournamentId))
        .Times(1)
        .WillOnce(Return(matches));

    testing::internal::CaptureStdout();
    fx.delegate.ProcessScoreUpdate(evt);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("Score update for tournament: " + evt.tournamentId),
              std::string::npos);

    EXPECT_EQ(out.find("Still pending group matches"), std::string::npos);
}
