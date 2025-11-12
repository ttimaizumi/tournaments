#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "controller/MatchController.hpp"
#include "delegate/IMatchDelegate.hpp"

using nlohmann::json;
using ::testing::_;
using ::testing::Return;

class MockMatchDelegate : public IMatchDelegate {
public:
    MOCK_METHOD((std::expected<json,std::string>), CreateMatch,
                (const std::string_view&, const json&), (override));
    MOCK_METHOD((std::expected<std::vector<json>,std::string>), GetMatches,
                (const std::string_view&, std::optional<std::string>), (override));
    MOCK_METHOD((std::expected<json,std::string>), GetMatch,
                (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD((std::expected<void,std::string>), UpdateScore,
                (const std::string_view&, const std::string_view&, int, int), (override));
};

TEST(MatchControllerTest, CreateMatch_Valid_Returns201) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    json body{
            {"bracket","winners"},
            {"round",1},
            {"homeTeamId","A"},
            {"visitorTeamId","B"}
    };
    crow::request req;
    req.body = body.dump();

    json created = body;
    created["id"] = "m1";
    EXPECT_CALL(*d, CreateMatch("t1", _))
        .WillOnce(Return(std::expected<json,std::string>{created}));

    auto res = c.CreateMatch(req, "t1");
    EXPECT_EQ(res.code, crow::CREATED);
}

TEST(MatchControllerTest, PatchScore_Tie_Returns422) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    json body{{"score", json{{"home",1},{"visitor",1}}}};
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*d, UpdateScore("t1","m1",1,1))
        .WillOnce(Return(std::unexpected(std::string{"invalid-score"})));

    auto res = c.PatchScore(req, "t1", "m1");
    EXPECT_EQ(res.code, 422);
}
