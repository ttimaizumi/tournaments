#ifndef SERVICE_IQUEUE_MESSAGE_PRODUCER_HPP
#define SERVICE_IQUEUE_MESSAGE_PRODUCER_HPP

#include <string_view>

class IQueueMessageProducer
{
public:
    virtual ~IQueueMessageProducer() = default;
    virtual void SendMessage(const std::string& message, const std::string& queue) = 0;
};
 

#endif /* SERVICE_IQUEUE_MESSAGE_PRODUCER_HPP */
