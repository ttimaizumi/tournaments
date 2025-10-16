#include <stdexcept>

class DuplicateException : public std::runtime_error {
public:
    explicit DuplicateException(const std::string& msg)
        : std::runtime_error(msg) {}
};