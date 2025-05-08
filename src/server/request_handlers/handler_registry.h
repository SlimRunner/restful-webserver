#ifndef HANDLER_REGISTRY_H
#define HANDLER_REGISTRY_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include "request_handler.h"

/*
This defines the signature of a factory function:
Given a config path and arguments, return a RequestHandler
*/
using RequestHandlerFactory = std::function<
    std::shared_ptr<RequestHandler>(const std::string& path_prefix,
                                    const std::map<std::string, std::string>& args)>;


/*
This means only one registry in the program
Allows handlers to register their factory functions by name 
Allows the server to look up the handlers for use later
*/
class HandlerRegistry {
    public:
    // Access the single shared instance of the registry
    static HandlerRegistry& instance() {
        static HandlerRegistry registry;
        return registry;
    }

    // Register a handler class with its factory function
    void register_handler(const std::string& name, RequestHandlerFactory factory) {
        factories_[name] = factory;
    }

    // Retrieve a factory by name
    RequestHandlerFactory get_factory(const std::string& name) const {
        auto it = factories_.find(name);
        if (it != factories_.end()) {
            return it->second;
        }
        return nullptr;
    }

    private:
    std::map<std::string, RequestHandlerFactory> factories_;
};

/*
This macro essentially gets the name of the handler, and the class it is and then adds it into the registry like a map
where the name is the name we have in the config file (e.g. EchoHandler, StaticHandler) and maps it to a lambda that calls 
the constructor of that class (e.g. EchoHandler, StaticFileHandler)
*/
#define REGISTER_HANDLER_WITH_NAME(CLASS, NAME) \
    static bool _##CLASS##_registered = []() { \
        HandlerRegistry::instance().register_handler(NAME, [](const std::string& path_prefix, const std::map<std::string, std::string>& args) { \
            return std::make_shared<CLASS>(path_prefix, args); \
        }); \
        return true; \
    }();

#endif  // HANDLER_REGISTRY_H
