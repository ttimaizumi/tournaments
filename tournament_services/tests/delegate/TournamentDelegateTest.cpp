#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <stdexcept>
#include <memory>
#include "delegate/TournamentDelegate.hpp"
#include "../mocks/TournamentRepositoryMock.h"

using namespace testing;
using namespace std::literals;

TEST(TournamentDelegateTest, CreateTournament_Success) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    EXPECT_CALL(*repo, Create(::testing::_))
        .WillOnce(Return("T123"s));  // FIX: string, not string_view
    TournamentDelegate sut{repo};
    auto res = sut.CreateTournament(std::make_shared<domain::Tournament>("Liga", domain::TournamentFormat{1,4,domain::TournamentType::NFL}));
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), "T123");
}

TEST(TournamentDelegateTest, CreateTournament_RepoThrows_ReturnsUnexpected) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    EXPECT_CALL(*repo, Create(::testing::_))
    .WillOnce(::testing::Throw(std::runtime_error("db error")));
    TournamentDelegate sut{repo};
    auto res = sut.CreateTournament(std::make_shared<domain::Tournament>("Liga", domain::TournamentFormat{1,4,domain::TournamentType::NFL}));
    ASSERT_FALSE(res.has_value());
    EXPECT_THAT(res.error(), HasSubstr("db error"));
}

TEST(TournamentDelegateTest, ReadById_Found_ReturnsTournament) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    auto t = std::make_shared<domain::Tournament>("X", domain::TournamentFormat{1,2,domain::TournamentType::NFL});
    EXPECT_CALL(*repo, ReadById("T1"))
        .WillOnce(Return(t));
    TournamentDelegate sut{repo};
    auto result = sut.ReadById("T1");
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);
    EXPECT_EQ(result.value()->Name(), "X");
}

TEST(TournamentDelegateTest, ReadById_Throws_ReturnsUnexpected) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    EXPECT_CALL(*repo, ReadById("T1"))
    .WillOnce(::testing::Throw(std::runtime_error("boom")));
    TournamentDelegate sut{repo};
    auto result = sut.ReadById("T1");
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), HasSubstr("boom"));
}

TEST(TournamentDelegateTest, ReadAll_Empty_ReturnsOkEmptyVector) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
    TournamentDelegate sut{repo};
    auto result = sut.ReadAll();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(TournamentDelegateTest, ReadAll_WithItems_ReturnsList) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    std::vector<std::shared_ptr<domain::Tournament>> tournaments {
        std::make_shared<domain::Tournament>("Alpha", domain::TournamentFormat{1,2,domain::TournamentType::NFL}),
        std::make_shared<domain::Tournament>("Beta",  domain::TournamentFormat{1,2,domain::TournamentType::NFL})
    };
    EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(tournaments));
    TournamentDelegate sut{repo};
    auto result = sut.ReadAll();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2u);
    EXPECT_EQ((*result)[0]->Name(), "Alpha");
    EXPECT_EQ((*result)[1]->Name(), "Beta");
}

TEST(TournamentDelegateTest, ReadAll_RepoThrows_ReturnsUnexpected) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    EXPECT_CALL(*repo, ReadAll())
      .WillOnce(::testing::Throw(std::runtime_error("boom")));
    TournamentDelegate sut{repo};
    auto result = sut.ReadAll();
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), HasSubstr("boom"));
}

TEST(TournamentDelegateTest, UpdateTournament_Success_ReturnsTrue) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();

    auto existing = std::make_shared<domain::Tournament>("Old", domain::TournamentFormat{1,4,domain::TournamentType::NFL});
    EXPECT_CALL(*repo, ReadById("U1"))
        .WillOnce(Return(existing));  // ensure found

    EXPECT_CALL(*repo, Update(::testing::_))
        .WillOnce(Return("U1"s));     // or .Times(1) if Update is void

    TournamentDelegate sut{repo};
    auto result = sut.UpdateTournament("U1", domain::Tournament{"Liga", domain::TournamentFormat{1,4,domain::TournamentType::NFL}});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}


TEST(TournamentDelegateTest, UpdateTournament_Throws_ReturnsUnexpected) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();

    auto existing = std::make_shared<domain::Tournament>("Old", domain::TournamentFormat{1,4,domain::TournamentType::NFL});
    EXPECT_CALL(*repo, ReadById("U1"))
        .WillOnce(Return(existing));  // ensure found so Update is reached

    EXPECT_CALL(*repo, Update(::testing::_))
        .WillOnce(::testing::Invoke([](const domain::Tournament&) -> std::string {
            throw std::runtime_error("db");
        }));
    // Si Update es void: use ThrowAction via lambda -> void

    TournamentDelegate sut{repo};
    auto result = sut.UpdateTournament("U1", domain::Tournament{"Liga", domain::TournamentFormat{1,4,domain::TournamentType::NFL}});
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), ::testing::HasSubstr("db"));
}


TEST(TournamentDelegateTest, DeleteTournament_Success_ReturnsTrue) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    auto t = std::make_shared<domain::Tournament>("N", domain::TournamentFormat{1,1,domain::TournamentType::NFL});
    EXPECT_CALL(*repo, ReadById("D2")).WillOnce(Return(t));
    EXPECT_CALL(*repo, Delete("D2")).Times(1);
    TournamentDelegate sut{repo};
    auto result = sut.DeleteTournament("D2");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}

TEST(TournamentDelegateTest, DeleteTournament_Throws_ReturnsUnexpected) {
    auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
    auto t = std::make_shared<domain::Tournament>("N", domain::TournamentFormat{1,1,domain::TournamentType::NFL});
    EXPECT_CALL(*repo, ReadById("D2")).WillOnce(Return(t));
    EXPECT_CALL(*repo, Delete("D2"))
        .WillOnce(::testing::Invoke([](const std::string&) -> void {
            throw std::runtime_error("fk");
        }));
    TournamentDelegate sut{repo};
    auto result = sut.DeleteTournament("D2");
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), ::testing::HasSubstr("fk"));
}

