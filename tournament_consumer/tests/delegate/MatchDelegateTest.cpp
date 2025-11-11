#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "domain/Match.hpp"
#include "domain/Team.hpp"
#include "domain/Group.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"

namespace {
    class MatchRepositoryMock : public IMatchRepository {
    public:
        MOCK_METHOD(std::string, Create, (const domain::Match&), (override));
        MOCK_METHOD(std::shared_ptr<domain::Match>, ReadById, (std::string), (override));
        MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, ReadAll, (), (override));
        MOCK_METHOD(std::string, Update, (const domain::Match&), (override));
        MOCK_METHOD(void, Delete, (std::string), (override));
        MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByTournamentId, (const std::string_view&), (override));
        MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindPlayedByTournamentId, (const std::string_view&), (override));
        MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindPendingByTournamentId, (const std::string_view&), (override));
        MOCK_METHOD(std::shared_ptr<domain::Match>, FindByTournamentIdAndMatchId, (const std::string_view&, const std::string_view&), (override));
        MOCK_METHOD(std::shared_ptr<domain::Match>, FindLastOpenMatch, (const std::string_view&), (override));
        MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindMatchesByTournamentAndRound, (const std::string_view&, domain::Round), (override));
        MOCK_METHOD(bool, TournamentExists, (const std::string_view&), (override));
    };

    class GroupRepositoryMock : public GroupRepository {
    public:
        GroupRepositoryMock() : GroupRepository(nullptr) {}
        MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId, (const std::string_view&, const std::string_view&), (override));
        MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId, (const std::string_view&), (override));
    };

    class TournamentRepositoryMock : public TournamentRepository {
    public:
        TournamentRepositoryMock() : TournamentRepository(nullptr) {}
    };

    class ConsumerMatchDelegateTest : public ::testing::Test {
    protected:
        std::shared_ptr<MatchRepositoryMock> matchRepoMock;
        std::shared_ptr<GroupRepositoryMock> groupRepoMock;
        std::shared_ptr<TournamentRepositoryMock> tournamentRepoMock;
        std::shared_ptr<MatchDelegate> matchDelegate;

        void SetUp() override {
            matchRepoMock = std::make_shared<MatchRepositoryMock>();
            groupRepoMock = std::make_shared<GroupRepositoryMock>();
            tournamentRepoMock = std::make_shared<TournamentRepositoryMock>();
            matchDelegate = std::make_shared<MatchDelegate>(matchRepoMock, groupRepoMock, tournamentRepoMock);
        }

        void TearDown() override {
            testing::Mock::VerifyAndClearExpectations(matchRepoMock.get());
            testing::Mock::VerifyAndClearExpectations(groupRepoMock.get());
        }
    };

    // ProcessTeamAddition tests

    TEST_F(ConsumerMatchDelegateTest, ProcessTeamAddition_GroupWith4Teams_CreatesMatches) {
        domain::TeamAddEvent event;
        event.tournamentId = "tournament-1";
        event.groupId = "group-A";
        event.teamId = "team-4";

        auto group = std::make_shared<domain::Group>();
        group->SetId("group-A");
        std::vector<domain::Team> teams = {
            {"team-1", "Team A"},
            {"team-2", "Team B"},
            {"team-3", "Team C"},
            {"team-4", "Team D"}
        };
        group->SetTeams(teams);

        EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(
                testing::Eq(std::string_view("tournament-1")),
                testing::Eq(std::string_view("group-A"))))
                .WillOnce(testing::Return(group));

        EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(
                testing::Eq(std::string_view("tournament-1")),
                testing::Eq(domain::Round::REGULAR)))
                .WillOnce(testing::Return(std::vector<std::shared_ptr<domain::Match>>{}));

        // Expect 6 matches to be created (4 teams round-robin: C(4,2) = 6)
        EXPECT_CALL(*matchRepoMock, Create(testing::_))
                .Times(6)
                .WillRepeatedly(testing::Return("match-id"));

        matchDelegate->ProcessTeamAddition(event);
    }

    TEST_F(ConsumerMatchDelegateTest, ProcessTeamAddition_GroupWith3Teams_DoesNotCreateMatches) {
        domain::TeamAddEvent event;
        event.tournamentId = "tournament-1";
        event.groupId = "group-A";
        event.teamId = "team-3";

        auto group = std::make_shared<domain::Group>();
        group->SetId("group-A");
        std::vector<domain::Team> teams = {
            {"team-1", "Team A"},
            {"team-2", "Team B"},
            {"team-3", "Team C"}
        };
        group->SetTeams(teams);

        EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(testing::_, testing::_))
                .WillOnce(testing::Return(group));

        // Should not try to create matches
        EXPECT_CALL(*matchRepoMock, Create(testing::_))
                .Times(0);

        matchDelegate->ProcessTeamAddition(event);
    }

    TEST_F(ConsumerMatchDelegateTest, ProcessTeamAddition_GroupNotFound_DoesNothing) {
        domain::TeamAddEvent event;
        event.tournamentId = "tournament-1";
        event.groupId = "non-existent";
        event.teamId = "team-1";

        EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(testing::_, testing::_))
                .WillOnce(testing::Return(nullptr));

        EXPECT_CALL(*matchRepoMock, Create(testing::_))
                .Times(0);

        matchDelegate->ProcessTeamAddition(event);
    }

    // ProcessScoreUpdate tests - Regular matches

    TEST_F(ConsumerMatchDelegateTest, ProcessScoreUpdate_RegularMatch_ChecksIfAllComplete) {
        domain::ScoreUpdateEvent event;
        event.tournamentId = "tournament-1";
        event.matchId = "match-1";
        event.score = {2, 1};

        auto match = std::make_shared<domain::Match>();
        match->SetId("match-1");
        match->SetRound(domain::Round::REGULAR);
        match->SetScore(event.score);

        EXPECT_CALL(*matchRepoMock, FindByTournamentIdAndMatchId(
                testing::Eq(std::string_view("tournament-1")),
                testing::Eq(std::string_view("match-1"))))
                .WillOnce(testing::Return(match));

        // Mock regular matches - only 1 complete (not all 48)
        std::vector<std::shared_ptr<domain::Match>> regularMatches;
        for (int i = 0; i < 10; ++i) {
            auto m = std::make_shared<domain::Match>();
            m->SetRound(domain::Round::REGULAR);
            regularMatches.push_back(m);
        }

        EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(
                testing::Eq(std::string_view("tournament-1")),
                testing::Eq(domain::Round::REGULAR)))
                .WillOnce(testing::Return(regularMatches));

        matchDelegate->ProcessScoreUpdate(event);
    }

    // ProcessScoreUpdate tests - Playoff matches

    TEST_F(ConsumerMatchDelegateTest, ProcessScoreUpdate_EighthsMatch_WaitsForAllBeforeCreatingQuarters) {
        domain::ScoreUpdateEvent event;
        event.tournamentId = "tournament-1";
        event.matchId = "match-1";
        event.score = {2, 1};

        auto match = std::make_shared<domain::Match>();
        match->SetId("match-1");
        match->SetHomeTeamId("team-1");
        match->SetHomeTeamName("Team A");
        match->SetVisitorTeamId("team-2");
        match->SetVisitorTeamName("Team B");
        match->SetRound(domain::Round::EIGHTHS);
        match->SetScore(event.score);

        EXPECT_CALL(*matchRepoMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
                .WillOnce(testing::Return(match));

        // Return 8 matches but only 7 have scores
        std::vector<std::shared_ptr<domain::Match>> eighthsMatches;
        for (int i = 0; i < 8; ++i) {
            auto m = std::make_shared<domain::Match>();
            m->SetRound(domain::Round::EIGHTHS);
            if (i < 7) {
                m->SetScore({i, i + 1});
            }
            eighthsMatches.push_back(m);
        }

        EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(
                testing::Eq(std::string_view("tournament-1")),
                testing::Eq(domain::Round::EIGHTHS)))
                .WillOnce(testing::Return(eighthsMatches));

        // Should not create quarters yet
        EXPECT_CALL(*matchRepoMock, Create(testing::_))
                .Times(0);

        matchDelegate->ProcessScoreUpdate(event);
    }

    TEST_F(ConsumerMatchDelegateTest, ProcessScoreUpdate_LastEighthsMatch_CreatesQuarters) {
        domain::ScoreUpdateEvent event;
        event.tournamentId = "tournament-1";
        event.matchId = "match-8";
        event.score = {2, 1};

        auto match = std::make_shared<domain::Match>();
        match->SetId("match-8");
        match->SetHomeTeamId("team-15");
        match->SetHomeTeamName("Team O");
        match->SetVisitorTeamId("team-16");
        match->SetVisitorTeamName("Team P");
        match->SetRound(domain::Round::EIGHTHS);
        match->SetScore(event.score);

        EXPECT_CALL(*matchRepoMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
                .WillOnce(testing::Return(match));

        // Return 8 eighths matches, all with scores
        std::vector<std::shared_ptr<domain::Match>> eighthsMatches;
        for (int i = 0; i < 8; ++i) {
            auto m = std::make_shared<domain::Match>();
            m->SetId("match-" + std::to_string(i + 1));
            m->SetHomeTeamId("team-h-" + std::to_string(i));
            m->SetHomeTeamName("Home Team " + std::to_string(i));
            m->SetVisitorTeamId("team-v-" + std::to_string(i));
            m->SetVisitorTeamName("Visitor Team " + std::to_string(i));
            m->SetRound(domain::Round::EIGHTHS);
            m->SetScore({i % 2 == 0 ? 2 : 1, i % 2 == 0 ? 1 : 2});
            eighthsMatches.push_back(m);
        }

        EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(
                testing::Eq(std::string_view("tournament-1")),
                testing::Eq(domain::Round::EIGHTHS)))
                .Times(2)  // Called twice: once to check, once to create quarters
                .WillRepeatedly(testing::Return(eighthsMatches));

        // Should create 4 quarters matches
        EXPECT_CALL(*matchRepoMock, Create(testing::_))
                .Times(4)
                .WillRepeatedly(testing::Return("quarter-match-id"));

        matchDelegate->ProcessScoreUpdate(event);
    }

    TEST_F(ConsumerMatchDelegateTest, ProcessScoreUpdate_LastQuarterMatch_CreatesSemis) {
        domain::ScoreUpdateEvent event;
        event.tournamentId = "tournament-1";
        event.matchId = "match-4";
        event.score = {3, 2};

        auto match = std::make_shared<domain::Match>();
        match->SetId("match-4");
        match->SetHomeTeamId("team-7");
        match->SetHomeTeamName("Team G");
        match->SetVisitorTeamId("team-8");
        match->SetVisitorTeamName("Team H");
        match->SetRound(domain::Round::QUARTERS);
        match->SetScore(event.score);

        EXPECT_CALL(*matchRepoMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
                .WillOnce(testing::Return(match));

        // Return 4 quarters matches, all with scores
        std::vector<std::shared_ptr<domain::Match>> quartersMatches;
        for (int i = 0; i < 4; ++i) {
            auto m = std::make_shared<domain::Match>();
            m->SetId("quarter-" + std::to_string(i + 1));
            m->SetHomeTeamId("team-h-" + std::to_string(i));
            m->SetHomeTeamName("QHome " + std::to_string(i));
            m->SetVisitorTeamId("team-v-" + std::to_string(i));
            m->SetVisitorTeamName("QVisitor " + std::to_string(i));
            m->SetRound(domain::Round::QUARTERS);
            m->SetScore({i % 2 == 0 ? 2 : 1, i % 2 == 0 ? 1 : 2});
            quartersMatches.push_back(m);
        }

        EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(
                testing::Eq(std::string_view("tournament-1")),
                testing::Eq(domain::Round::QUARTERS)))
                .Times(2)
                .WillRepeatedly(testing::Return(quartersMatches));

        // Should create 2 semis matches
        EXPECT_CALL(*matchRepoMock, Create(testing::_))
                .Times(2)
                .WillRepeatedly(testing::Return("semi-match-id"));

        matchDelegate->ProcessScoreUpdate(event);
    }

    TEST_F(ConsumerMatchDelegateTest, ProcessScoreUpdate_LastSemiMatch_CreatesFinal) {
        domain::ScoreUpdateEvent event;
        event.tournamentId = "tournament-1";
        event.matchId = "match-2";
        event.score = {1, 0};

        auto match = std::make_shared<domain::Match>();
        match->SetId("match-2");
        match->SetHomeTeamId("team-3");
        match->SetHomeTeamName("Team C");
        match->SetVisitorTeamId("team-4");
        match->SetVisitorTeamName("Team D");
        match->SetRound(domain::Round::SEMIS);
        match->SetScore(event.score);

        EXPECT_CALL(*matchRepoMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
                .WillOnce(testing::Return(match));

        // Return 2 semis matches, both with scores
        std::vector<std::shared_ptr<domain::Match>> semisMatches;
        for (int i = 0; i < 2; ++i) {
            auto m = std::make_shared<domain::Match>();
            m->SetId("semi-" + std::to_string(i + 1));
            m->SetHomeTeamId("team-h-" + std::to_string(i));
            m->SetHomeTeamName("SHome " + std::to_string(i));
            m->SetVisitorTeamId("team-v-" + std::to_string(i));
            m->SetVisitorTeamName("SVisitor " + std::to_string(i));
            m->SetRound(domain::Round::SEMIS);
            m->SetScore({i == 0 ? 2 : 1, i == 0 ? 1 : 2});
            semisMatches.push_back(m);
        }

        EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(
                testing::Eq(std::string_view("tournament-1")),
                testing::Eq(domain::Round::SEMIS)))
                .Times(2)
                .WillRepeatedly(testing::Return(semisMatches));

        // Should create 1 final match
        EXPECT_CALL(*matchRepoMock, Create(testing::_))
                .Times(1)
                .WillOnce(testing::Return("final-match-id"));

        matchDelegate->ProcessScoreUpdate(event);
    }

    TEST_F(ConsumerMatchDelegateTest, ProcessScoreUpdate_FinalMatch_DeclaresChampion) {
        domain::ScoreUpdateEvent event;
        event.tournamentId = "tournament-1";
        event.matchId = "final-match";
        event.score = {3, 1};

        auto match = std::make_shared<domain::Match>();
        match->SetId("final-match");
        match->SetHomeTeamId("team-1");
        match->SetHomeTeamName("Champion Team");
        match->SetVisitorTeamId("team-2");
        match->SetVisitorTeamName("Runner-up Team");
        match->SetRound(domain::Round::FINAL);
        match->SetScore(event.score);

        EXPECT_CALL(*matchRepoMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
                .WillOnce(testing::Return(match));

        // Should not create any more matches
        EXPECT_CALL(*matchRepoMock, Create(testing::_))
                .Times(0);

        matchDelegate->ProcessScoreUpdate(event);
    }

    TEST_F(ConsumerMatchDelegateTest, ProcessScoreUpdate_MatchNotFound_DoesNothing) {
        domain::ScoreUpdateEvent event;
        event.tournamentId = "tournament-1";
        event.matchId = "non-existent";
        event.score = {1, 0};

        EXPECT_CALL(*matchRepoMock, FindByTournamentIdAndMatchId(testing::_, testing::_))
                .WillOnce(testing::Return(nullptr));

        matchDelegate->ProcessScoreUpdate(event);
    }
}