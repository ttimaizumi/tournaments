#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "delegate/MatchDelegate.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"

#include <optional>
#include <expected>


using nlohmann::json;
using ::testing::_;
using ::testing::Return;

class MockMatchRepository : public IMatchRepository {
public:
    MOCK_METHOD(std::vector<json>, FindByTournament,
        (std::string_view, std::optional<std::string>), (override));

    MOCK_METHOD(std::optional<json>, FindByTournamentAndId,
        (std::string_view, std::string_view), (override));

    MOCK_METHOD(std::optional<std::string>, Create,
        (const json&), (override));

    MOCK_METHOD(bool, UpdateScore,
        (std::string_view, std::string_view, const json&, std::string), (override));

    MOCK_METHOD(bool, UpdateParticipants,
        (std::string_view, std::string_view,
         std::optional<std::string>, std::optional<std::string>), (override));
};

class MockProducer : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage,
                (const std::string& message, const std::string& queue),
                (override));
};


TEST(MatchDelegateTest, CreateMatch_SendsCreatedEvent) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    json body{{"bracket","winners"},{"round",1}};

    EXPECT_CALL(*repo, Create(_))
        .WillOnce(Return(std::optional<std::string>{"m1"}));
    EXPECT_CALL(*prod, SendMessage("m1", "match.created"))
        .Times(1);

    auto r = d.CreateMatch("t1", body);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->at("id"), "m1");
}

TEST(MatchDelegateTest, UpdateScore_Advances_NoTie) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    json doc{
        {"id","m1"},
        {"tournamentId","t1"},
        {"homeTeamId","A"},
        {"visitorTeamId","B"},
        {"status","scheduled"},
        {"score", json{{"home",0},{"visitor",0}}},
        {"advancement", json{
            {"winner", json{{"matchId","m2"},{"slot","home"}}},
            {"loser",  json{{"matchId","m3"},{"slot","visitor"}}}
        }}
    };

    // El delegate debe encontrar el match original
    EXPECT_CALL(*repo, FindByTournamentAndId("t1","m1"))
        .WillOnce(Return(std::optional<json>{doc}));

    // Debe guardar el score como "played"
    EXPECT_CALL(*repo, UpdateScore("t1","m1",_, "played"))
        .WillOnce(Return(true));

    // Ganador A -> m2.home ; Perdedor B -> m3.visitor
    // Usamos '_' en los opcionales para evitar el problema con std::nullopt.
    EXPECT_CALL(*repo,
        UpdateParticipants("t1", "m2",
                           std::optional<std::string>("A"), _))
        .WillOnce(Return(true));

    EXPECT_CALL(*repo,
        UpdateParticipants("t1", "m3",
                           _, std::optional<std::string>("B")))
        .WillOnce(Return(true));

    // Eventos esperados
    EXPECT_CALL(*prod, SendMessage("m2", "match.advanced")).Times(1);
    EXPECT_CALL(*prod, SendMessage("m3", "match.advanced")).Times(1);
    EXPECT_CALL(*prod, SendMessage("m1", "match.score-recorded")).Times(1);

    auto r = d.UpdateScore("t1","m1", 2, 1);
    EXPECT_TRUE(r.has_value());
}

// ======================================================================
// Tests extra para MatchDelegate
// ======================================================================

// Test: si el repositorio falla al crear el match (Create devuelve nullopt),
// el delegate regresa expected sin valor y no manda evento match.created.
TEST(MatchDelegateTest, CreateMatch_RepoError_ReturnsUnexpected) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    json body{{"bracket","winners"},{"round",1}};

    EXPECT_CALL(*repo, Create(_))
        .WillOnce(Return(std::optional<std::string>{})); // simulamos fallo BD

    EXPECT_CALL(*prod, SendMessage(_, _)).Times(0);

    auto r = d.CreateMatch("t1", body);
    EXPECT_FALSE(r.has_value());
}

// Test: GetMatch cuando el partido existe.
// Debe regresar el json con el id correcto.
TEST(MatchDelegateTest, GetMatch_Found_ReturnsMatch) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    json stored{
        {"id","m1"},
        {"tournamentId","t1"},
        {"homeTeamId","A"},
        {"visitorTeamId","B"}
    };

    EXPECT_CALL(*repo, FindByTournamentAndId("t1","m1"))
        .WillOnce(Return(std::optional<json>{stored}));

    auto r = d.GetMatch("t1","m1");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->at("id"), "m1");
}

// Test: GetMatch cuando el partido NO existe.
// Debe regresar expected sin valor y no llamar UpdateScore ni UpdateParticipants.
TEST(MatchDelegateTest, GetMatch_NotFound_ReturnsUnexpected) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    EXPECT_CALL(*repo, FindByTournamentAndId("t1","missing"))
        .WillOnce(Return(std::optional<json>{}));

    auto r = d.GetMatch("t1","missing");
    EXPECT_FALSE(r.has_value());
}

// Test: GetMatches sin filtro (showMatches vacio) regresa la lista de partidos
// que entregue el repositorio.
TEST(MatchDelegateTest, GetMatches_NoFilter_ReturnsList) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    std::vector<json> matches{
        json{{"id","m1"}},
        json{{"id","m2"}}
    };

    // No nos importa tanto el filtro, solo que se llame FindByTournament
    EXPECT_CALL(*repo, FindByTournament("t1", _))
        .WillOnce(Return(matches));

    auto r = d.GetMatches("t1", std::nullopt);
    ASSERT_TRUE(r.has_value());
    const auto &v = r.value();
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].at("id"), "m1");
    EXPECT_EQ(v[1].at("id"), "m2");
}

// Test: GetMatches con filtro "played" debe pasar ese filtro al repositorio.
// Aqui solo verificamos que se llama FindByTournament con el optional correcto.
TEST(MatchDelegateTest, GetMatches_FilterPlayed_PassesFilterToRepo) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    std::vector<json> matches{}; // no nos interesa el contenido

    EXPECT_CALL(*repo,
        FindByTournament("t1", std::optional<std::string>{"played"}))
        .WillOnce(Return(matches));

    auto r = d.GetMatches("t1", std::optional<std::string>{"played"});
    EXPECT_TRUE(r.has_value());
}

// Test: UpdateScore cuando el partido NO existe.
// Debe regresar expected sin valor y no tocar nada mas.
TEST(MatchDelegateTest, UpdateScore_MatchNotFound_ReturnsUnexpected) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    EXPECT_CALL(*repo, FindByTournamentAndId("t1","mX"))
        .WillOnce(Return(std::optional<json>{}));

    EXPECT_CALL(*repo, UpdateScore(_,_,_,_)).Times(0);
    EXPECT_CALL(*repo, UpdateParticipants(_,_,_,_)).Times(0);
    EXPECT_CALL(*prod, SendMessage(_, _)).Times(0);

    auto r = d.UpdateScore("t1","mX", 2, 1);
    EXPECT_FALSE(r.has_value());
}

// Test: UpdateScore con empate.
// Debe rechazar el score (expected sin valor) y NO tocar repositorio ni producer.
TEST(MatchDelegateTest, UpdateScore_TieWithAdvancement_IsRejected) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    // En la implementacion actual, para empates el delegate
    // regresa error antes de consultar el repositorio,
    // asi que NO esperamos ninguna llamada al repo.

    EXPECT_CALL(*repo, FindByTournamentAndId(_, _)).Times(0);
    EXPECT_CALL(*repo, UpdateScore(_, _, _, _)).Times(0);
    EXPECT_CALL(*repo, UpdateParticipants(_, _, _, _)).Times(0);
    EXPECT_CALL(*prod, SendMessage(_, _)).Times(0);

    auto r = d.UpdateScore("t1","mTie", 1, 1);

    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "invalid-score");
}


// Test: UpdateScore para un partido "normal" sin campo advancement.
// Debe solo marcar el partido como played y mandar evento match.score-recorded
// sin generar partidos siguientes.
TEST(MatchDelegateTest, UpdateScore_NoAdvancement_OnlyRecordsScore) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    json doc{
        {"id","mRegular"},
        {"tournamentId","t1"},
        {"homeTeamId","A"},
        {"visitorTeamId","B"},
        {"status","scheduled"},
        {"score", json{{"home",0},{"visitor",0}}}
    };

    EXPECT_CALL(*repo, FindByTournamentAndId("t1","mRegular"))
        .WillOnce(Return(std::optional<json>{doc}));

    EXPECT_CALL(*repo, UpdateScore("t1","mRegular", _, "played"))
        .WillOnce(Return(true));

    EXPECT_CALL(*repo, UpdateParticipants(_,_,_,_)).Times(0);
    EXPECT_CALL(*prod, SendMessage("mRegular", "match.score-recorded"))
        .Times(1);

    auto r = d.UpdateScore("t1","mRegular", 3, 1);
    EXPECT_TRUE(r.has_value());
}

// Test: UpdateScore con advancement solo ganador (caso tipico de eliminacion
// sencilla donde solo se pasa el ganador al siguiente partido).
// Debe actualizar score, avanzar al siguiente y mandar eventos.
TEST(MatchDelegateTest, UpdateScore_OnlyWinnerAdvancement_UpdatesNextMatch) {
    auto repo = std::make_shared<MockMatchRepository>();
    auto prod = std::make_shared<MockProducer>();
    MatchDelegate d(repo, prod);

    json doc{
        {"id","mWin"},
        {"tournamentId","t1"},
        {"homeTeamId","H"},
        {"visitorTeamId","V"},
        {"status","scheduled"},
        {"score", json{{"home",0},{"visitor",0}}},
        {"advancement", json{
            {"winner", json{{"matchId","mNext"},{"slot","visitor"}}}
        }}
    };

    EXPECT_CALL(*repo, FindByTournamentAndId("t1","mWin"))
        .WillOnce(Return(std::optional<json>{doc}));

    EXPECT_CALL(*repo, UpdateScore("t1","mWin", _, "played"))
        .WillOnce(Return(true));

    EXPECT_CALL(*repo,
        UpdateParticipants("t1", "mNext", _, std::optional<std::string>{"H"}))
        .WillOnce(Return(true));

    EXPECT_CALL(*prod, SendMessage("mNext", "match.advanced")).Times(1);
    EXPECT_CALL(*prod, SendMessage("mWin",  "match.score-recorded")).Times(1);

    auto r = d.UpdateScore("t1","mWin", 2, 0);
    EXPECT_TRUE(r.has_value());
}
