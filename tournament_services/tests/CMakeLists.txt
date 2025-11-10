project(tournament_tests)
set(TEST_COMMON_LIBS
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        tournament_common
)

include_directories(../include)

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

# Single test executable with all test files
add_executable(tournament_services_tests
        controller/GroupControllerTest.cpp
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/MatchControllerTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/MatchDelegateTest.cpp
        ../src/controller/GroupController.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/controller/MatchController.cpp
        ../src/delegate/GroupDelegate.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
        ../src/delegate/MatchDelegate.cpp
)

target_link_libraries(tournament_services_tests PRIVATE ${TEST_COMMON_LIBS})
gtest_discover_tests(tournament_services_tests)