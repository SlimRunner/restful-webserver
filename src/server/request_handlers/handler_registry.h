#ifndef HANDLER_REGISTRY_H
#define HANDLER_REGISTRY_H

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "request_handler.h"

struct RoutingPayload {
    std::string handler;
    std::map<std::string, std::string> arguments;
};

/// @brief handler factory signature
using RequestHandlerFactory = std::function<std::shared_ptr<RequestHandler>(
    const std::string& path_prefix, const std::map<std::string, std::string>& args)>;

/// @brief resource map to retrieve factories
using RoutingMap = std::map<std::string, RoutingPayload>;

/// @brief Used to manage singleton dependency injection
class IHandlerRegistry {
   public:
    virtual ~IHandlerRegistry() = default;
    virtual void register_handler(const std::string& name, RequestHandlerFactory factory) = 0;
    virtual RequestHandlerFactory get_factory(const std::string& name) const = 0;
};

/// @brief Singleton registry to manage handler factories by name
class HandlerRegistry : public IHandlerRegistry {
   public:
    // Access the single shared instance of the registry
    static HandlerRegistry& instance() {
        static HandlerRegistry registry;
        return registry;
    }

    // Register a handler class with its factory function
    void register_handler(const std::string& name, RequestHandlerFactory factory) override;

    // Retrieve a factory by name
    RequestHandlerFactory get_factory(const std::string& name) const override;

   private:
    // the block below sets up this class as a proper singleton.

    // constructor must be private
    HandlerRegistry() = default;
    // disable copy semantics
    HandlerRegistry(const HandlerRegistry&) = delete;
    HandlerRegistry& operator=(const HandlerRegistry&) = delete;
    // disable move semantics
    HandlerRegistry(HandlerRegistry&&) = delete;
    HandlerRegistry& operator=(HandlerRegistry&&) = delete;

    std::map<std::string, RequestHandlerFactory> factories_;
};

/*
This macro essentially gets the name of the handler, and the class it is and then adds it into the
registry like a map where the name is the name we have in the config file (e.g. EchoHandler,
StaticHandler) and maps it to a lambda that calls the constructor of that class (e.g. EchoHandler,
StaticFileHandler)
*/
#define REGISTER_HANDLER_WITH_NAME(CLASS, NAME)                                                  \
    static bool _##CLASS##_registered = []() {                                                   \
        HandlerRegistry::instance().register_handler(                                            \
            NAME,                                                                                \
            [](const std::string& path_prefix, const std::map<std::string, std::string>& args) { \
                return std::make_shared<CLASS>(path_prefix, args);                               \
            });                                                                                  \
        return true;                                                                             \
    }();

#endif  // HANDLER_REGISTRY_H
