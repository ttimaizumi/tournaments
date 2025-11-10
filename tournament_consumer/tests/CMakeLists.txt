project(consumer_tests)
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
add_executable(tournament_consumer_tests
        delegate/MatchDelegateTest.cpp
        listener/ScoreUpdateListenerTest.cpp
        ../src/delegate/MatchDelegate.cpp
)

target_link_libraries(tournament_consumer_tests PRIVATE ${TEST_COMMON_LIBS})
gtest_discover_tests(tournament_consumer_tests)
