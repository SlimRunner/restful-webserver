#ifndef TAGGED_EXCEPTIONS_H
#define TAGGED_EXCEPTIONS_H

#include <stdexcept>

namespace expt {

// meaningless exception used as template for tagged ones so that we
// need less unit tests here
class base_tagged_exception : public std::exception {
   public:
    base_tagged_exception(const char* message);

    base_tagged_exception(const std::string& message);

    const char* what() const noexcept override;

   private:
    std::string msg_;
};

struct config_exception : public base_tagged_exception {
    using base_tagged_exception::base_tagged_exception;
};

}  // namespace expt

#endif
