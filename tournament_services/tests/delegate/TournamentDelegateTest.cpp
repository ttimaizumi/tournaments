//
// Created by root on 10/9/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "domain/Tournament.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "delegate/TournamentDelegate.hpp"

namespace {
    class TournamentRepositoryMock : public IRepository<domain::Tournament, std::string> {
    public:
        MOCK_METHOD(std::string, Create, (const domain::Tournament&), (override));
        MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string), (override));
        MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
        MOCK_METHOD(std::string, Update, (const domain::Tournament&), (override));
        MOCK_METHOD(void, Delete, (std::string), (override));
    };

    class QueueMessageProducerMock : public QueueMessageProducer {
    public:
        QueueMessageProducerMock() : QueueMessageProducer(nullptr) {}

        MOCK_METHOD(void, SendMessage, (const std::string_view&, const std::string_view&), (override));
    };

    class TournamentDelegateTest : public ::testing::Test {
    protected:
        std::shared_ptr<TournamentRepositoryMock> repositoryMockPtr;
        std::shared_ptr<QueueMessageProducerMock> producerMockPtr;
        TournamentRepositoryMock* repositoryMock;
        QueueMessageProducerMock* producerMock;
        std::shared_ptr<TournamentDelegate> tournamentDelegate;

        void SetUp() override {
            repositoryMockPtr = std::make_shared<TournamentRepositoryMock>();
            producerMockPtr = std::make_shared<QueueMessageProducerMock>();
            repositoryMock = repositoryMockPtr.get();
            producerMock = producerMockPtr.get();

            tournamentDelegate = std::shared_ptr<TournamentDelegate>(
                    new TournamentDelegate(repositoryMockPtr, producerMockPtr)
            );
        }

        void TearDown() override {
            testing::Mock::VerifyAndClearExpectations(repositoryMock);
            testing::Mock::VerifyAndClearExpectations(producerMock);
        }
    };

    // Test 1: Crear torneo - validar transferencia y respuesta con ID generado
    TEST_F(TournamentDelegateTest, CreateTournament_ValidInsertion_ReturnsGeneratedId) {
        auto tournament = std::make_shared<domain::Tournament>("Mundial 2025");
        const std::string expectedId = "generated-uuid-123";
        domain::Tournament capturedTournament("");

        EXPECT_CALL(*repositoryMock, Create(testing::_))
                .WillOnce(testing::DoAll(
                        testing::SaveArg<0>(&capturedTournament),
                        testing::Return(expectedId)
                ));

        EXPECT_CALL(*producerMock, SendMessage(testing::Eq(expectedId), testing::Eq("tournament.created")))
                .Times(1);

        auto result = tournamentDelegate->CreateTournament(tournament);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(expectedId, result.value());
        EXPECT_EQ("Mundial 2025", capturedTournament.Name());
    }

    // Test 2: Crear torneo - inserción fallida retorna std::unexpected
    TEST_F(TournamentDelegateTest, CreateTournament_FailedInsertion_ReturnsError) {
        auto tournament = std::make_shared<domain::Tournament>("Duplicate Tournament");

        EXPECT_CALL(*repositoryMock, Create(testing::_))
                .WillOnce(testing::Throw(std::runtime_error("Duplicate entry")));

        EXPECT_CALL(*producerMock, SendMessage(testing::_, testing::_))
                .Times(0);

        auto result = tournamentDelegate->CreateTournament(tournament);

        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error(), testing::HasSubstr("Error creating tournament"));
    }

    // Test 3: Buscar por ID - validar transferencia de ID y retorno con objeto
    TEST_F(TournamentDelegateTest, ReadById_ValidId_ReturnsObjectWithCorrectValues) {
        const std::string tournamentId = "tournament-uuid-456";
        std::string capturedId;
        auto expectedTournament = std::make_shared<domain::Tournament>("Mundial 2025");
        expectedTournament->Id() = tournamentId;

        EXPECT_CALL(*repositoryMock, ReadById(testing::_))
                .WillOnce(testing::DoAll(
                        testing::SaveArg<0>(&capturedId),
                        testing::Return(expectedTournament)
                ));

        auto result = tournamentDelegate->ReadById(tournamentId);

        EXPECT_EQ(tournamentId, capturedId);
        ASSERT_TRUE(result.has_value());
        auto tournament = result.value();
        ASSERT_NE(nullptr, tournament);
        EXPECT_EQ(tournamentId, tournament->Id());
        EXPECT_EQ("Mundial 2025", tournament->Name());
    }

    // Test 4: Buscar por ID - validar transferencia de ID y resultado nulo
    TEST_F(TournamentDelegateTest, ReadById_InvalidId_ReturnsNullptr) {
        const std::string invalidId = "non-existent-id";
        std::string capturedId;

        EXPECT_CALL(*repositoryMock, ReadById(testing::_))
                .WillOnce(testing::DoAll(
                        testing::SaveArg<0>(&capturedId),
                        testing::Return(nullptr)
                ));

        auto result = tournamentDelegate->ReadById(invalidId);

        EXPECT_EQ(invalidId, capturedId);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(nullptr, result.value());
    }

    // Test 5: Buscar todos - resultado con lista de objetos
    TEST_F(TournamentDelegateTest, ReadAll_WithTournaments_ReturnsListOfObjects) {
        auto tournament1 = std::make_shared<domain::Tournament>("Mundial 2025");
        tournament1->Id() = "id-1";
        auto tournament2 = std::make_shared<domain::Tournament>("Copa America");
        tournament2->Id() = "id-2";
        auto tournament3 = std::make_shared<domain::Tournament>("Eurocopa");
        tournament3->Id() = "id-3";

        std::vector<std::shared_ptr<domain::Tournament>> tournaments = {
            tournament1, tournament2, tournament3
    };

        EXPECT_CALL(*repositoryMock, ReadAll())
                .WillOnce(testing::Return(tournaments));

        auto result = tournamentDelegate->ReadAll();

        ASSERT_TRUE(result.has_value());
        auto tournamentList = result.value();
        ASSERT_EQ(3, tournamentList.size());
        EXPECT_EQ("id-1", tournamentList[0]->Id());
        EXPECT_EQ("Mundial 2025", tournamentList[0]->Name());
        EXPECT_EQ("id-2", tournamentList[1]->Id());
        EXPECT_EQ("Copa America", tournamentList[1]->Name());
        EXPECT_EQ("id-3", tournamentList[2]->Id());
        EXPECT_EQ("Eurocopa", tournamentList[2]->Name());
    }

    // Test 6: Buscar todos - resultado con lista vacía
    TEST_F(TournamentDelegateTest, ReadAll_EmptyRepository_ReturnsEmptyList) {
        std::vector<std::shared_ptr<domain::Tournament>> emptyList;

        EXPECT_CALL(*repositoryMock, ReadAll())
                .WillOnce(testing::Return(emptyList));

        auto result = tournamentDelegate->ReadAll();

        ASSERT_TRUE(result.has_value());
        auto tournamentList = result.value();
        EXPECT_TRUE(tournamentList.empty());
        EXPECT_EQ(0, tournamentList.size());
    }

    // Test 7: Actualizar torneo - validar transferencia a Update y resultado exitoso
    TEST_F(TournamentDelegateTest, UpdateTournament_ValidUpdate_ReturnsUpdatedId) {
        const std::string tournamentId = "tournament-uuid-789";
        domain::Tournament tournament("Updated Tournament Name");
        tournament.Id() = tournamentId;
        domain::Tournament capturedTournament("");

        EXPECT_CALL(*repositoryMock, Update(testing::_))
                .WillOnce(testing::DoAll(
                        testing::SaveArg<0>(&capturedTournament),
                        testing::Return(tournamentId)
                ));

        auto result = tournamentDelegate->UpdateTournament(tournament);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(tournamentId, result.value());
        EXPECT_EQ(tournamentId, capturedTournament.Id());
        EXPECT_EQ("Updated Tournament Name", capturedTournament.Name());
    }

    // Test 8: Actualizar torneo - búsqueda no exitosa retorna std::unexpected
    TEST_F(TournamentDelegateTest, UpdateTournament_NonExistentId_ReturnsError) {
        const std::string nonExistentId = "non-existent-uuid";
        domain::Tournament tournament("Some Tournament");
        tournament.Id() = nonExistentId;

        EXPECT_CALL(*repositoryMock, Update(testing::_))
                .WillOnce(testing::Throw(std::runtime_error("Tournament not found")));

        auto result = tournamentDelegate->UpdateTournament(tournament);

        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error(), testing::HasSubstr("Error updating tournament"));
    }
}