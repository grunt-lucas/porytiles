#ifndef PORYTILES_TSEXCEPTION_H
#define PORYTILES_TSEXCEPTION_H

#include <string>
#include <stdexcept>

namespace porytiles {
// Generic porytiles exception class
class TsException : public std::runtime_error {
public:
    explicit TsException(const std::string& msg) : std::runtime_error{msg} {}
};
} // namespace porytiles

#endif // PORYTILES_TSEXCEPTION_H