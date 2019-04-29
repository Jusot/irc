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

    bool with_prefix() const;

    std::string nick() const;
    std::string user() const;
    std::string hostname() const;

    std::string source() const;
    std::string command() const;
    std::vector<std::string> args() const;

private:
    bool with_prefix_;

    std::string nick_;
    std::string user_;
    std::string hostname_;

    std::string source_;
    std::string command_;
    std::vector<std::string> args_;
};
}

#endif // NPCP_MESSAGE_HPP