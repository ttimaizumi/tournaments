// main.cpp
#include <crow.h>
#include <activemq/library/ActiveMQCPP.h>

#include "configuration/ContainerSetup.hpp"
#include "configuration/RunConfiguration.hpp"
#include "configuration/RouteDefinition.hpp"  // <-- necesario para routeRegistry()

// RAII para asegurar shutdown de ActiveMQ aunque haya excepcion
struct ActiveMQGuard {
    ActiveMQGuard()  { activemq::library::ActiveMQCPP::initializeLibrary(); }
    ~ActiveMQGuard() { activemq::library::ActiveMQCPP::shutdownLibrary();   }
    ActiveMQGuard(const ActiveMQGuard&) = delete;
    ActiveMQGuard& operator=(const ActiveMQGuard&) = delete;
};

int main() {
    ActiveMQGuard amq_guard;

    // DI container
    auto container = config::containerSetup();

    // App Crow
    crow::SimpleApp app;

    // Enlazar todas las rutas registradas via REGISTER_ROUTE(...)
    // Nota: asegúrate de NO definir NO_ROUTE_REGISTRATION en este binario
    for (auto& def : routeRegistry()) {
        def.binder(app, container);
    }

    // Cargar configuracion de ejecucion
    auto appConfig = container->resolve<config::RunConfiguration>();

    // Levantar servidor
    app.port(appConfig->port)
       .concurrency(appConfig->concurrency)
       .run();

    return 0;
}
