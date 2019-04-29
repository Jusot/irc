#include <mutex>
#include <string>
#include <unordered_map>

#include "users.hpp"
#include "message.hpp
#include "../icarus/icarus/buffer.hpp"

using namespace icarus;

namespace npcp
{
std::unordered_map<std::string, TcpConnectionPtr> nick_conn;
std::unordered_map<TcpConnectionPtr, Session> conn_session;

std::mutex nick_conn_mutex;
std::mutex conn_session_mutex;

void on_message(const TcpConnectionPtr& conn, Buffer* buf)
{
    Message message(buf->retrieve_all_as_string());

    const auto &source = message.source(), ins = message.ins();
    const auto &args = message.args();

    if (ins == "NICK")
    {
        if (args().empty())
        {
            // RETURN ERR_NONNICKNAMEGIVEN
        }
        else if (nick_conn.count(args().front()))
        {
            // RETURN ERR_NICKNAMEINUSE
        }
        else if (conn_nick.count(conn))
        {
            {
                std::lock_guard(nick_conn_mutex);
                nick_conn.erase(conn_nick[conn]);
                nick_conn[args.front()] = conn;
            }

            {
                std::lock_guard(conn_session_mutex);
                conn_session[conn] = { args.front(), conn_session[conn].realname };
            }

            // RETURN PRL_WECLOME
        }
        else
        {
            {
                std::lock_guard(nick_conn_mutex);
                nick_conn[args.front()] = nullptr;
            }
        }
    }
    else if (ins == "USER")
    {
        if (conn_nick.count(conn))
        {
            // RETURN ERR_ALREADYREGISTRED
        }
        else if (args().size() != 4)
        {
            // RETURN ERR_NEEDMOREPARAMS
        }
        else
        {
            {
                std::lock_guard(conn_session_mutex);
                conn_session[conn] = { args[0], args[3] };
            }
            // RETURN RPL_WELCOME
        }
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
