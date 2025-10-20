cmake_minimum_required(VERSION 3.22)

project(tournament_tests)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# =================== GoogleTest + GoogleMock ===================
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# =================== nlohmann_json (header-only) ===================
FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# =================== Fuentes de TESTS ===================
set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/GroupControllerTest.cpp
        controller/MatchControllerTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/MatchDelegateTest.cpp
        delegate/StandingDelegateTest.cpp
        delegate/EliminationDelegateTest.cpp
        consumer/GroupTeamAddedConsumerTest.cpp
        consumer/MatchFinishedConsumerTest.cpp
)


set(DELEGATE_SOURCES)

# =================== Includes base ===================
# Solo usamos stubs de tests/support/ - NO incluimos ../include para evitar conflictos
include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}/support         # tests/support/ con todos los stubs
)

#Ejecutable de tests
add_executable(${PROJECT_NAME}_runner
        ${TEST_SOURCES}
        ${DELEGATE_SOURCES}
)

# Evitar dependencias de Crow/registro de rutas
target_compile_definitions(${PROJECT_NAME}_runner PRIVATE
        NO_ROUTE_REGISTRATION
        UNIT_TEST
        ASIO_STANDALONE
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gmock_main
        GTest::gmock
        GTest::gtest
        nlohmann_json::nlohmann_json
        # tournament_common  # Descomenta SOLO si existe como target CMake real
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_runner)

# =================== Individual test files ===================
# Función para agregar tests individuales
function(add_tournament_test test_name test_source)
    add_executable(${test_name} ${test_source})

    target_compile_definitions(${test_name} PRIVATE
        NO_ROUTE_REGISTRATION
        UNIT_TEST
        ASIO_STANDALONE
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
    )

    target_link_libraries(${test_name} PRIVATE
        GTest::gmock_main
        GTest::gmock
        GTest::gtest
        nlohmann_json::nlohmann_json
    )

    gtest_discover_tests(${test_name})
endfunction()

# Individual test files
add_tournament_test(team_controller_test controller/TeamControllerTest.cpp)
add_tournament_test(tournament_controller_test controller/TournamentControllerTest.cpp)
add_tournament_test(group_controller_test controller/GroupControllerTest.cpp)
add_tournament_test(match_controller_test controller/MatchControllerTest.cpp)
add_tournament_test(team_delegate_test delegate/TeamDelegateTest.cpp)
add_tournament_test(tournament_delegate_test delegate/TournamentDelegateTest.cpp)
add_tournament_test(group_delegate_test delegate/GroupDelegateTest.cpp)
add_tournament_test(match_delegate_test delegate/MatchDelegateTest.cpp)
add_tournament_test(standing_delegate_test delegate/StandingDelegateTest.cpp)
add_tournament_test(elimination_delegate_test delegate/EliminationDelegateTest.cpp)
add_tournament_test(group_team_added_consumer_test consumer/GroupTeamAddedConsumerTest.cpp)
add_tournament_test(match_finished_consumer_test consumer/MatchFinishedConsumerTest.cpp)
