//
// Created by root on 11/3/25.
//

#ifndef TOURNAMENTS_HEALTHCONTROLLER_HPP
#define TOURNAMENTS_HEALTHCONTROLLER_HPP

#include "configuration/RouteDefinition.hpp"

class HealthController {
    public:
    crow::response GetHealth(){
        return crow::response{crow::OK, "Services running"};
    }
};

REGISTER_ROUTE(HealthController, GetHealth, "/health", "GET"_method)
#endif //TOURNAMENTS_HEALTHCONTROLLER_HPP