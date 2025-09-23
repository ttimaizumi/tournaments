project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        ../src/controller/TeamController.cpp) # Not sure if this is the correct solution. Wont compile without directly linking the source.

set(SOURCES ${SERVICES_SOURCES} ${TEST_SOURCES})

find_package(GTest CONFIG REQUIRED)

include_directories(../include)

add_executable(${PROJECT_NAME}_runner
    ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        tournament_common)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)