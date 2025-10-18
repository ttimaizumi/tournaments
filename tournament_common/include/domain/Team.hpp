//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_DOMAIN_TEAM_HPP
#define RESTAPI_DOMAIN_TEAM_HPP
#include <string>

namespace domain {
    struct Team {
        std::string Id;
        std::string Name;
    };
}
#endif //RESTAPI_DOMAIN_TEAM_HPP