//
// Created by tomas on 9/13/25.
//

#ifndef SERVICE_IRESOLVER_HPP
#define SERVICE_IRESOLVER_HPP
#include <memory>

template<typename T>
class IResolver {
public:
    virtual ~IResolver() = default;
    virtual std::shared_ptr<T> Resolve(const std::string_view& key) = 0;
    virtual std::shared_ptr<T> Resolve() = 0;
};

#endif //SERVICE_IRESOLVER_HPP