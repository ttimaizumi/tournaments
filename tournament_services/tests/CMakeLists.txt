cmake_minimum_required(VERSION 3.22)

project(tournament_tests)

set(CMAKE_CXX_STANDARD 23)

set(TEST_SOURCES
        # tests
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/GroupControllerTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/GroupDelegateTest.cpp

        # units under test (compiled into the test runner)
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/controller/GroupController.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
        ../src/delegate/GroupDelegate.cpp
)

# headers from services and common
include_directories(../include)
include_directories(../../tournament_common/include)

find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_runner ${TEST_SOURCES})

# do not register HTTP routes when building tests
target_compile_definitions(${PROJECT_NAME}_runner PRIVATE NO_ROUTE_REGISTRATION)

# use a single gtest main (gmock_main), and link common lib
target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gmock_main
        GTest::gmock
        GTest::gtest
        tournament_common
)

add_test(NAME AllTests COMMAND ${PROJECT_NAME}_runner)
