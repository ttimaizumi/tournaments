
#include <activemq/library/ActiveMQCPP.h>

#include "include/configuration/ContainerSetup.hpp"
#include "include/configuration/RunConfiguration.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    const auto container = config::containerSetup();
    crow::SimpleApp app;

    // Bind all annotated routes
    for (auto& def : routeRegistry()) {
        def.binder(app, container);
    }

    auto appConfig = container->resolve<config::RunConfiguration>();

    app.port(appConfig->port)b
        .concurrency(appConfig->concurrency)
        .run();
    activemq::library::ActiveMQCPP::shutdownLibrary();
}
