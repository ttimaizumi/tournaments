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
// ======================================================================
// Tests extra para MatchController
// ======================================================================

// Test: si el delegate regresa error "tournament-not-found" al crear un match,
// el controller debe responder HTTP 404.
TEST(MatchControllerTest, CreateMatch_TournamentNotFound_Returns500) {
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

    EXPECT_CALL(*d, CreateMatch("t1", _))
        .WillOnce(Return(std::unexpected(std::string{"tournament-not-found"})));

    auto res = c.CreateMatch(req, "t1");
    EXPECT_EQ(res.code, 500);
}

// Test: si el delegate regresa algun error generico (ej. "db-error"),
// el controller debe responder HTTP 500.
TEST(MatchControllerTest, CreateMatch_DelegateError_Returns500) {
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

    EXPECT_CALL(*d, CreateMatch("t1", _))
        .WillOnce(Return(std::unexpected(std::string{"db-error"})));

    auto res = c.CreateMatch(req, "t1");
    EXPECT_EQ(res.code, 500);
}

// Test: PatchScore exitoso, sin error del delegate,
// debe regresar HTTP 204 (sin contenido).
TEST(MatchControllerTest, PatchScore_Success_Returns204) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    json body{{"score", json{{"home",2},{"visitor",1}}}};
    crow::request req;
    req.body = body.dump();

    EXPECT_CALL(*d, UpdateScore("t1","m1",2,1))
        .WillOnce(Return(std::expected<void,std::string>{}));

    auto res = c.PatchScore(req, "t1", "m1");
    EXPECT_EQ(res.code, 204);
}

// Test: PatchScore cuando el delegate regresa "match-not-found",
// debe mapear a HTTP 404.
TEST(MatchControllerTest, PatchScore_MatchNotFound_Returns404) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    json body{{"score", json{{"home",2},{"visitor",1}}}};
    crow::request req;
    req.body = body.dump();

    EXPECT_CALL(*d, UpdateScore("t1","m1",2,1))
        .WillOnce(Return(std::unexpected(std::string{"match-not-found"})));

    auto res = c.PatchScore(req, "t1", "m1");
    EXPECT_EQ(res.code, 404);
}

// Test: PatchScore cuando el delegate regresa un error distinto
// de "invalid-score" y "match-not-found" (ej. "db-error"),
// debe responder HTTP 500.
TEST(MatchControllerTest, PatchScore_UnexpectedError_Returns500) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    json body{{"score", json{{"home",3},{"visitor",1}}}};
    crow::request req;
    req.body = body.dump();

    EXPECT_CALL(*d, UpdateScore("t1","m1",3,1))
        .WillOnce(Return(std::unexpected(std::string{"db-error"})));

    auto res = c.PatchScore(req, "t1", "m1");
    EXPECT_EQ(res.code, 500);
}
// =========================
// Tests para GET /tournaments/{id}/matches
// =========================

TEST(MatchControllerTest, GetMatches_Success_Returns200) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    std::vector<json> matches = {
        json{{"id","m1"}},
        json{{"id","m2"}}
    };

    EXPECT_CALL(*d, GetMatches("t1", ::testing::_))
        .WillOnce(Return(std::expected<std::vector<json>,std::string>{matches}));

    crow::request req; // sin query param status
    auto res = c.GetMatches(req, "t1");

    EXPECT_EQ(res.code, 200);
}

TEST(MatchControllerTest, GetMatches_DelegateError_Returns500) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    EXPECT_CALL(*d, GetMatches("t1", ::testing::_))
        .WillOnce(Return(std::unexpected(std::string{"db-error: fail"})));

    crow::request req;
    auto res = c.GetMatches(req, "t1");

    EXPECT_EQ(res.code, 500);
}

// =========================
// Tests para GET /tournaments/{id}/matches/{matchId}
// =========================

TEST(MatchControllerTest, GetMatch_InvalidId_Returns400) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    // matchId invalido (espacios, caracteres raros, etc.)
    auto res = c.GetMatch("t1", "id con espacios");

    EXPECT_EQ(res.code, 400);
}

TEST(MatchControllerTest, GetMatch_NotFound_Returns404) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    EXPECT_CALL(*d, GetMatch("t1","m1"))
        .WillOnce(Return(std::unexpected(std::string{"not-found"})));

    auto res = c.GetMatch("t1", "m1");

    EXPECT_EQ(res.code, 404);
}

TEST(MatchControllerTest, GetMatch_DelegateError_Returns500) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    EXPECT_CALL(*d, GetMatch("t1","m1"))
        .WillOnce(Return(std::unexpected(std::string{"db-error: x"})));

    auto res = c.GetMatch("t1", "m1");

    EXPECT_EQ(res.code, 500);
}

TEST(MatchControllerTest, GetMatch_Success_Returns200) {
    auto d = std::make_shared<MockMatchDelegate>();
    MatchController c(d);

    json match{
        {"id","m1"},
        {"tournamentId","t1"}
    };

    EXPECT_CALL(*d, GetMatch("t1","m1"))
        .WillOnce(Return(std::expected<json,std::string>{match}));

    auto res = c.GetMatch("t1", "m1");

    EXPECT_EQ(res.code, 200);
}

