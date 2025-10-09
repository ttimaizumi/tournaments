project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
)

set(SOURCES ${TEST_SOURCES})

# Headers de tournament_services (y opcionalmente tournament_common)
include_directories(../include)
# include_directories(../../tournament_common/include)  # si lo necesitas

find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_runner
        ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        tournament_common
)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)
