project(tournament_common)

set(CMAKE_CXX_STANDARD 23)
set(COMMON_SOURCES
        src/persistence/repository/TeamRepository.cpp
        src/persistence/repository/TournamentRepository.cpp
        src/persistence/repository/GroupRepository.cpp
)

include_directories(include)

add_library(${PROJECT_NAME} STATIC ${COMMON_SOURCES})

target_include_directories(${PROJECT_NAME}
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
)