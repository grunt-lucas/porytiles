#ifndef PORYTILES_PTEXCEPTION_H
#define PORYTILES_PTEXCEPTION_H

#include <string>
#include <stdexcept>

/**
 * An exception class for Porytiles. This exception typically represents something that went wrong due to user error.
 * For internal errors that signal code bugs, Porytiles will throw one of the standard C++ exception types.
 */

namespace porytiles {
// Generic porytiles exception class
class PtException : public std::runtime_error {
public:
    explicit PtException(const std::string& msg) : std::runtime_error{msg} {}
};
} // namespace porytiles

#endif // PORYTILES_PTEXCEPTION_H