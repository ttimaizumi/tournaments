project(tournament_tests)
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

set(TEST_COMMON_LIBS
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        tournament_common
)

include_directories(../include)

add_executable(test_runner
        controller/GroupControllerTest.cpp
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
        ../src/controller/GroupController.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/delegate/GroupDelegate.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
)

target_link_libraries(test_runner PRIVATE ${TEST_COMMON_LIBS})
gtest_discover_tests(test_runner)
