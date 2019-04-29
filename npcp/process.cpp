#include <mutex>
#include <string>
#include <unordered_map>

#include "message.hpp"
#include "process.hpp"
#include "../icarus/icarus/buffer.hpp"
#include "../icarus/icarus/tcpconnection.hpp"

using namespace icarus;

namespace
{
static const std::string _hostname = ":jusot.com";

constexpr std::size_t cal_hash(const char* str)
{
    return (*str == '\0') ? 0 : ((cal_hash(str + 1) * 201314 + *str) % 5201314);
}

constexpr std::size_t operator""_hash(const char* str, std::size_t)
{
    return cal_hash(str);
}
}

namespace npcp
{
std::unordered_map<std::string, TcpConnectionPtr> nick_conn;
std::unordered_map<TcpConnectionPtr, Session> conn_session;
std::unordered_map<std::string, std::vector<std::string>> channels;

std::mutex nick_conn_mutex;

static bool _check_registered(const TcpConnectionPtr& conn)
{
    return conn_session.count(conn) && nick_conn.count(conn_session[conn].nickname);
}

static void _ins_nick_process(const TcpConnectionPtr&, const std::string&, const std::string&, const std::vector<std::string>&);
static bool _ins_user_process(const TcpConnectionPtr&, const std::string&, const std::string&, const std::vector<std::string>&);
static bool _ins_quit_process(const TcpConnectionPtr&, const std::string&, const std::string&, const std::vector<std::string>&);

void on_message(const TcpConnectionPtr& conn, Buffer* buf)
{
    Message message(buf->retrieve_all_as_string());

    const auto& source = message.source(), ins = message.ins();
    const auto& args = message.args();

    auto hs = cal_hash(ins.c_str());

    switch (hs)
    {
    case "NICK"_hash:
        _ins_nick_process(conn, source, ins, args);
        break;

    case "USER"_hash:
        _ins_user_process(conn, source, ins, args);
        break;

#define RPL_WHEN_NOTREGISTERED if (!_check_registered(conn)) \ 
        { \
            conn->send(Reply::gen_reply({_hostname, \
                "451", \
                ":You have not registered"})); \
            break; \
        }   // 451 for ERR_NOTREGISTERED

    case "QUIT"_hash:
        RPL_WHEN_NOTREGISTERED;
        _ins_quit_process(conn, source, ins, args);
        break;

    case "WHOIS"_hash:
        RPL_WHEN_NOTREGISTERED;
        break;

    case "MODE"_hash:
        RPL_WHEN_NOTREGISTERED;
        break;

    default:
        if (_check_registered(conn))
        {
            conn->send(Reply::gen_reply({_hostname,
                "421",  // 421 for ERR_UNKNOWNCOMMAND
                ins,    // <command>
                ":Unknown command"}));
        }
        break;
    }
}

static void _ins_nick_process(const TcpConnectionPtr& conn,
    const std::string& source,
    const std::string& ins,
    const std::vector<std::string>& args)
{
    std::lock_guard lock(nick_conn_mutex);

    if (args.empty())
    {
        conn->send(Reply::gen_reply({_hostname,
            "431",  // 431 for ERR_NONICKNAMEGIVEN
            ":No nickname given"}));
    }
    else if (nick_conn.count(args.front()))
    {
        conn->send(Reply::gen_reply({_hostname,
            "432",  // 432 for ERR_NICKNAMEINUSE
            args.front(), // <nick>
            ":Nickname is already in use"}));
    }
    else if (conn_session.count(conn))
    {
        if (nick_conn.count(conn_session[conn].nickname))
            nick_conn.erase(conn_session[conn].nickname);
        nick_conn[args.front()] = conn;
        conn_session[conn] = { args.front(), conn_session[conn].realname };

        // RETURN RPL_WELCOME
        // RETURN RPL_YOURHOST
        // RETURN RPL_CREATED
        // RETURN RPL_MYINFO
    }
    else
    {
        nick_conn[args.front()] = nullptr;
    }
}

static bool _ins_user_process(const TcpConnectionPtr& conn,
    const std::string& source,
    const std::string& ins,
    const std::vector<std::string>& args)
{
    std::lock_guard lock(nick_conn_mutex);

    if (conn_session.count(conn))
    {
        // RETURN ERR_ALREADYREGISTRED
    }
    else if (args.size() != 4)
    {
        // RETURN ERR_NEEDMOREPARAMS
    }
    else
    {
        conn_session[conn] = { args[0], args[3] };
        if (nick_conn.count(args[0]))
        {
            // RETURN RPL_WELCOME
            // RETURN RPL_YOURHOST
            // RETURN RPL_CREATED
            // RETURN RPL_MYINFO
        }
    }
}

static bool _ins_quit_process(const TcpConnectionPtr& conn,
    const std::string& source,
    const std::string& ins,
    const std::vector<std::string>& args)
{
    std::lock_guard lock(nick_conn_mutex);

    nick_conn.erase(conn_session[conn].nickname);
    conn_session.erase(conn);

    // RETURN Closing Link: HOSTNAME (MSG)
}

}
