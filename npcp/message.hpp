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
    std::string ins() const;
    std::vector<std::string> args() const;

private:
    std::string source_;
    std::string ins_;
    std::vector<std::string> args_;
};

class Reply
{
public:
    static std::string gen_reply(const std::vector<std::string>& args);
};
}

#endif // NPCP_MESSAGE_HPP