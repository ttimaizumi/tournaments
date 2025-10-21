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

# Macro to create one test target per file
function(add_tournament_test name)
    add_executable(${name}_runner
            ${ARGN}
            ../src/controller/TeamController.cpp
            ../src/controller/TournamentController.cpp
            ../src/delegate/TeamDelegate.cpp
            ../src/delegate/TournamentDelegate.cpp
    )
    target_link_libraries(${name}_runner PRIVATE ${TEST_COMMON_LIBS})
    gtest_discover_tests(${name}_runner)
endfunction()

# Individual test files
add_tournament_test(group_controller_test controller/GroupControllerTest.cpp)
add_tournament_test(team_controller_test controller/TeamControllerTest.cpp)
add_tournament_test(tournament_controller_test controller/TournamentControllerTest.cpp)
add_tournament_test(group_delegate_test delegate/GroupDelegateTest.cpp)
add_tournament_test(team_delegate_test delegate/TeamDelegateTest.cpp)
add_tournament_test(tournament_delegate_test delegate/TournamentDelegateTest.cpp)