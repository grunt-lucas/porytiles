#ifndef PORYTILES_PORYTILES_EXCEPTION_H
#define PORYTILES_PORYTILES_EXCEPTION_H

#include <stdexcept>
#include <string>

/**
 * An exception class for Porytiles. This exception typically represents something that went wrong due to user error.
 * For internal errors that signal code bugs, Porytiles will throw one of the standard C++ exception types.
 */

namespace porytiles {
// Generic porytiles exception class
class PorytilesException : public std::runtime_error {
public:
  explicit PorytilesException(const std::string &msg) : std::runtime_error{msg} {}
};
} // namespace porytiles

#endif // PORYTILES_PORYTILES_EXCEPTION_H
