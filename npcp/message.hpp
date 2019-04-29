#ifndef NPCP_MESSAGE_HPP
#define NPCP_MESSAGE_HPP

#include <string>
#include <vector>

namespace npcp
{
class Message
{
public:
    Message(std::string message);
    ~Message() = default;

    std::string source() const;
    std::string command() const;
    std::vector<std::string> args() const;

private:
    std::string source_;
    std::string command_;
    std::vector<std::string> args_;
};
}

#endif // NPCP_MESSAGE_HPP