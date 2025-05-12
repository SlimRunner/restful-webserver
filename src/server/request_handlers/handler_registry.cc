#include "handler_registry.h"

#include <boost/log/trivial.hpp>

void HandlerRegistry::register_handler(const std::string& name, RequestHandlerFactory factory) {
    factories_[name] = factory;
    BOOST_LOG_TRIVIAL(debug) << name << " was registered";
}

RequestHandlerFactory HandlerRegistry::get_factory(const std::string& name) const {
    auto it = factories_.find(name);
    BOOST_LOG_TRIVIAL(debug) << "looking for factory of " << name;
    if (it != factories_.end()) {
        BOOST_LOG_TRIVIAL(debug) << "factory found";
        return it->second;
    }
    BOOST_LOG_TRIVIAL(debug) << "factory not found";
    return nullptr;
}
