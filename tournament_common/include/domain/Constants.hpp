#ifndef TOURNAMENT_COMMON_CONSTANTS_HPP
#define TOURNAMENT_COMMON_CONSTANTS_HPP

#include <regex>

// UUID validation regex constant used throughout the project
static const std::regex ID_VALUE(R"(^[0-9a-fA-F]{8}\b-[0-9a-fA-F]{4}\b-[0-9a-fA-F]{4}\b-[0-9a-fA-F]{4}\b-[0-9a-fA-F]{12}$)");

#endif // TOURNAMENT_COMMON_CONSTANTS_HPP