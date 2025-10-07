//
// Created by developer on 8/21/25.
//

#ifndef RESTAPI_ROUTE_DEFINITION_HPP
#define RESTAPI_ROUTE_DEFINITION_HPP

#include <crow.h>
#include <Hypodermic/Container.h>
#include <vector>
#include <functional>
#include <string>

// Route definition storage
struct RouteDefinition {
    std::string path;
    crow::HTTPMethod method;
    std::function<void(crow::SimpleApp &, std::shared_ptr<Hypodermic::Container>)> binder;
};

inline std::vector<RouteDefinition> &routeRegistry() {
    static std::vector<RouteDefinition> registry;
    return registry;
}

template<typename Controller, typename Method, typename... Args>
auto invokeController(Controller* controller, Method method, const crow::request& request, Args&&... args) {
    if constexpr(std::is_invocable_v<Method, Controller*>) {
        return (controller->*method)();
    }
    else if constexpr( std::is_invocable_v<Method, Controller*, Args...>) {
        return (controller->*method)(std::forward<Args>(args)...);
    }
    else if constexpr( std::is_invocable_v<Method, Controller*, const crow::request&>) {
        return (controller->*method)(request);
    }
    if constexpr(std::is_invocable_v<Method, Controller*, const crow::request&, Args...>) {
        return (controller->*method)(request, std::forward<Args>(args)...);
    }
    else  {
        throw std::runtime_error("Controller does not exist");
    }

}

// Annotation-style macro
#define REGISTER_ROUTE(Controller, Method, Path, HttpMethod) \
struct Controller## _##Method##_RouteRegistrator { \
    Controller##_##Method##_RouteRegistrator() { \
        routeRegistry().push_back({ Path, HttpMethod, \
            [](crow::SimpleApp& app, const std::shared_ptr<Hypodermic::Container>& container) { \
                    CROW_ROUTE(app, Path).methods(HttpMethod)( \
                        [container](const crow::request& request ,auto&&... args) { \
                        auto controller = container->resolve<Controller>(); \
                        return invokeController(controller.get(), &Controller::Method, request, std::forward<decltype(args)>(args)...); \
                    } \
                ); \
            } \
        }); \
    } \
}; \
static Controller##_##Method##_RouteRegistrator global_##Controller##_##Method##_registrator;

#endif //RESTAPI_ROUTE_DEFINITION_HPP
