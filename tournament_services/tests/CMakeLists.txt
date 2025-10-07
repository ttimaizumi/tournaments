project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
)

set(SOURCES ${TEST_SOURCES})
include_directories(../include)

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