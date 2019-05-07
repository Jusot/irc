#include <algorithm>

#include "message.hpp"

using namespace npcp;

Message::Message(std::string message)
  : with_prefix_(false)
{
    {
        std::size_t bp= 0;
        while (bp < message.size() && message[bp] == ' ') ++bp;
        message = message.substr(bp);
    }

    raw_ = message;
    if (raw_.empty()) return;

    auto crlf_pos = raw_.find("\r\n");
    if (crlf_pos == std::string::npos) return;
    else if (crlf_pos > 510) crlf_pos = 510;

    while (crlf_pos >= 0 && raw_[crlf_pos - 1] == ' ') --crlf_pos;

    std::size_t pos = 0, end_pos;

    if (raw_[pos] == ':')
    {
        end_pos = raw_.find(' ', pos);
        if (end_pos != raw_.npos)
        {
            source_ = raw_.substr(pos + 1, end_pos - pos - 1);
            while (end_pos < raw_.size() && raw_[end_pos] == ' ') ++end_pos;
            pos = end_pos;

            with_prefix_ = true;
            auto t_pos = source_.find('!'), a_pos = source_.find('@');
            nick_ = source_.substr(0, t_pos);
            user_ = source_.substr(t_pos + 1, a_pos - t_pos);
            hostname_ = source_.substr(a_pos + 1);
        }
        else return;
    }

    end_pos = std::min(raw_.find(' ', pos), crlf_pos);
    command_ = raw_.substr(pos, end_pos - pos);
    while (++end_pos < crlf_pos && raw_[end_pos] == ' ');
    pos = end_pos;

    while (pos < crlf_pos)
    {
        if (raw_[pos] == ':')
        {
            args_.push_back(raw_.substr(pos + 1, crlf_pos - pos - 1));
            break;
        }
        end_pos = std::min(raw_.find(' ', pos), crlf_pos);
        args_.push_back(raw_.substr(pos, end_pos - pos));
        while (++end_pos < crlf_pos && raw_[end_pos] == ' ');
        pos = end_pos;
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