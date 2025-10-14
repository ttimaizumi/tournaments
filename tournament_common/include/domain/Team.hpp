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

        // Getter
        std::string& getName() { return Name; }
        const std::string& getName() const { return Name; }

        // Setter (optional but clean)
        void setName(const std::string& name) { Name = name; }
    };
}
#endif //RESTAPI_DOMAIN_TEAM_HPP