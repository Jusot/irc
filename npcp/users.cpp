#include <string>
#include <unordered_map>

#include "users.hpp"
#include "message.hpp
#include "../icarus/icarus/buffer.hpp"

using namespace icarus;

namespace npcp
{
std::unordered_map<std::string, TcpConnectionPtr> users;

void on_message(const TcpConnectionPtr& conn, Buffer* buf)
{
    Message message(buf->retrieve_all_as_string());

    auto ins = message.ins();
    if (ins == "NICK")
    {
        // ...
    }
    else if (ins == "WHOIS")
    {
        // ...
    }
    else if (ins == "MODE")
    {
        // ...
    }
    else if (ins == "QUIT")
    {
        // ...
    }
    else if (ins == "PRIVMSG")
    {
        // ...
    }
    else
    {
        // ...
    }
}
}
