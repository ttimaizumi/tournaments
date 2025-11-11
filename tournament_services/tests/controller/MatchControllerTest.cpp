//
// Created by edgar on 11/10/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <crow.h>
#include <nlohmann/json.hpp>

#include "controller/MatchController.hpp"
#include "delegate/IMatchDelegate.hpp"

using nlohmann::json;
using ::testing::_;
using ::testing::Return;

class MockMatchDelegate : public IMatchDelegate
{
public:
    MOCK_METHOD((std::expected<std::vector<json>, std::string>),
                GetMatches,
                (const std::string_view&, std::optional<std::string>), (override));

    MOCK_METHOD((std::expected<json, std::string>),
                GetMatch,
                (const std::string_view&, const std::string_view&), (override));

    MOCK_METHOD((std::expected<void, std::string>),
                UpdateScore,
                (const std::string_view&, const std::string_view&,
                 int, int), (override));
};

TEST(MatchControllerTest, GetMatch_NotFound_Returns404)
{
    auto delegate = std::make_shared<MockMatchDelegate>();
    MatchController controller(delegate);

    EXPECT_CALL(*delegate, GetMatch("t1", "m1"))
        .WillOnce(Return(std::unexpected(std::string{"match-not-found"})));

    auto res = controller.GetMatch("t1", "m1");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(MatchControllerTest, PatchScore_InvalidJson_Returns400)
{
    auto delegate = std::make_shared<MockMatchDelegate>();
    MatchController controller(delegate);

    crow::request req;
    req.body = "no es json";

    auto res = controller.PatchScore(req, "t1", "m1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(MatchControllerTest, PatchScore_InvalidScoreBusinessRule_Returns422)
{
    auto delegate = std::make_shared<MockMatchDelegate>();
    MatchController controller(delegate);

    crow::request req;
    json body{
        {"score", json{{"home", 1}, {"visitor", 1}}}
    };
    req.body = body.dump();

    EXPECT_CALL(*delegate, UpdateScore("t1", "m1", 1, 1))
        .WillOnce(Return(std::unexpected(std::string{"invalid-score"})));

    auto res = controller.PatchScore(req, "t1", "m1");
    EXPECT_EQ(res.code, 422);
}

TEST(MatchControllerTest, PatchScore_Success_Returns204)
{
    auto delegate = std::make_shared<MockMatchDelegate>();
    MatchController controller(delegate);

    crow::request req;
    json body{
        {"score", json{{"home", 2}, {"visitor", 1}}}
    };
    req.body = body.dump();

    EXPECT_CALL(*delegate, UpdateScore("t1", "m1", 2, 1))
        .WillOnce(Return(std::expected<void, std::string>{}));

    auto res = controller.PatchScore(req, "t1", "m1");
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}
