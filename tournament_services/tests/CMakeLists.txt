project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/GroupControllerTest.cpp
        controller/MatchControllerTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/MatchDelegateTest.cpp
        delegate/BracketGeneratorTest.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/controller/GroupController.cpp
        ../src/controller/MatchController.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
        ../src/delegate/GroupDelegate.cpp
        ../src/delegate/MatchDelegate.cpp
        ../../tournament_consumer/src/delegate/BracketGenerator.cpp
)

set(SOURCES ${TEST_SOURCES})
include_directories(../include)
include_directories(../../tournament_consumer/include)

find_package(GTest CONFIG REQUIRED)



add_executable(${PROJECT_NAME}_runner
    ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        tournament_common
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)