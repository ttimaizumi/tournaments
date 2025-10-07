#include <stdexcept>

class InvalidFormatException : public std::runtime_error {
public:
    explicit InvalidFormatException(const std::string& msg)
        : std::runtime_error(msg) {}
};