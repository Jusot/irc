#include <algorithm>

#include "message.hpp"

using namespace npcp;

Message::Message(std::string message)
  : with_prefix_(false),
    raw_(message)
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
            
            with_prefix_ = true;
            auto t_pos = source_.find('!'), a_pos = source_.find('@');
            nick_ = source_.substr(0, t_pos);
            user_ = source_.substr(t_pos + 1, a_pos - t_pos);
            hostname_ = source_.substr(a_pos + 1);
        }
        else return;
    }

    end_pos = std::min(message.find(' ', pos), crlf_pos);
    command_ = message.substr(pos, end_pos - pos);
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

bool Message::with_prefix() const
{
    return with_prefix_;
}

std::string Message::nick() const
{
    return nick_;
}

std::string Message::user() const
{
    return user_;
}

std::string Message::hostname() const
{
    return hostname_;
}

std::string Message::raw() const
{
    return raw_;
}

std::string Message::source() const
{
    return source_;
}

std::string Message::command() const
{
    return command_;
}

std::vector<std::string> Message::args() const
{
    return args_;
}