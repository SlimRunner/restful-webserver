#include "tagged_exceptions.h"

namespace expt {

// meaningless exception used as template for tagged ones so that we
// need less unit tests here
base_tagged_exception::base_tagged_exception(const char* message) : msg_(message) {}

base_tagged_exception::base_tagged_exception(const std::string& message) : msg_(message) {}

const char* base_tagged_exception::what() const noexcept {
    return msg_.c_str();
}

}  // namespace expt
