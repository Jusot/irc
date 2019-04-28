#include <algorithm>

#include "message.hpp"

using namespace npcp;

Message::Message(std::string message)
{
    if (message.empty()) return;

    auto crlf_pos = message.find("\r\n");
    if (crlf_pos == message.npos) return;
    else if (crlf_pos > 510) crlf_pos = 510;

    std::size_t pos = 0, end_pos;

    if (message[pos] == ':')
    {
        end_pos = message.find(' ', pos);
        if (end_pos != message.npos)
        {
            source_ = message.substr(pos + 1, end_pos - pos - 1);
            pos = end_pos + 1;
        }
        else return;
    }

    end_pos = std::min(message.find(' ', pos), crlf_pos);
    ins_ = message.substr(pos, end_pos - pos);
    pos = end_pos + 1;

    while (pos < crlf_pos)
    {
        if (message[pos] == ':')
        {
            args_.push_back(message.substr(pos + 1, crlf_pos - pos - 1));
            break;
        }
        end_pos = std::min(message.find(' ', pos), crlf_pos);
        args_.push_back(message.substr(pos, end_pos - pos));
        pos = end_pos + 1;
    }
}

std::string Message::source() const
{
    return source_;
}

std::string Message::ins() const
{
    return ins_;
}

std::vector<std::string> Message::args() const
{
    return args_;
}

std::string Reply::gen_reply(const std::vector<std::string> &args)
{
    std::string reply;
    for (const auto &arg : args) reply += arg + ' ';
    return reply + "\r\n";
}