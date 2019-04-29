#include <mutex>
#include <string>
#include <unordered_map>

#include "../icarus/icarus/buffer.hpp"
#include "../icarus/icarus/tcpconnection.hpp"

#include "message.hpp"
#include "process.hpp"
#include "rplfuncs.hpp"

using namespace icarus;

namespace
{
static const std::string _hostname = "jusot.com";

constexpr std::size_t cal_hash(const char *str)
{
    return (*str == '\0') ? 0 : ((cal_hash(str + 1) * 201314 + *str) % 5201314);
}

constexpr std::size_t operator""_hash(const char *str, std::size_t)
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

static bool _check_registered(const TcpConnectionPtr &conn)
{
    return conn_session.count(conn) && nick_conn.count(conn_session[conn].nickname);
}

static void _ins_nick_process(const TcpConnectionPtr&, const std::string&, const std::string&, const std::vector<std::string>&);
static bool _ins_user_process(const TcpConnectionPtr&, const std::string&, const std::string&, const std::vector<std::string>&);
static bool _ins_quit_process(const TcpConnectionPtr&, const std::string&, const std::string&, const std::vector<std::string>&);

void on_message(const TcpConnectionPtr &conn, Buffer *buf)
{
    Message message(buf->retrieve_all_as_string());

    const auto& source = message.source(), command = message.command();
    const auto& args = message.args();

    auto hs = cal_hash(command.c_str());

    switch (hs)
    {
    case "NICK"_hash:
        _ins_nick_process(conn, source, command, args);
        break;

    case "USER"_hash:
        _ins_user_process(conn, source, command, args);
        break;

#define RPL_WHEN_NOTREGISTERED if (!_check_registered(conn)) \ 
        { \
            conn->send(Reply::err_notregistered()); \
            break; \
        }

    case "QUIT"_hash:
        RPL_WHEN_NOTREGISTERED;
        _ins_quit_process(conn, source, command, args);
        break;

    case "WHOIS"_hash:
        RPL_WHEN_NOTREGISTERED;
        break;

    case "MODE"_hash:
        RPL_WHEN_NOTREGISTERED;
        break;

    default:
        if (_check_registered(conn))
            conn->send(Reply::err_unknowncommand(command));
        break;
    }
}

static void _ins_nick_process(const TcpConnectionPtr &conn,
    const std::string &source,
    const std::string &command,
    const std::vector<std::string> &args)
{
    std::lock_guard lock(nick_conn_mutex);

    if (args.empty())
        conn->send(Reply::err_nonicknamegiven());
    else if (nick_conn.count(args.front()))
        conn->send(Reply::err_nicknameinuse(args.front()));
    else if (conn_session.count(conn))
    {
        if (nick_conn.count(conn_session[conn].nickname))
            nick_conn.erase(conn_session[conn].nickname);
        nick_conn[args.front()] = conn;
        conn_session[conn] = { args.front(), conn_session[conn].realname };

        conn->send(Reply::rpl_welcome(source));
        conn->send(Reply::rpl_yourhost("2"));
        conn->send(Reply::rpl_created());
        conn->send(Reply::rpl_myinfo("2", "ao", "mtov"));
    }
    else nick_conn[args.front()] = nullptr;
}

static bool _ins_user_process(const TcpConnectionPtr &conn,
    const std::string &source,
    const std::string &command,
    const std::vector<std::string> &args)
{
    std::lock_guard lock(nick_conn_mutex);

    if (conn_session.count(conn))
        conn->send(Reply::err_alreadyregistered());
    else if (args.size() != 4)
        conn->send(Reply::err_needmoreparams(command));
    else
    {
        conn_session[conn] = { args[0], args[3] };
        if (nick_conn.count(args[0]))
        {
            conn->send(Reply::rpl_welcome(source));
            conn->send(Reply::rpl_yourhost("2"));
            conn->send(Reply::rpl_created());
            conn->send(Reply::rpl_myinfo("2", "ao", "mtov"));
        }
    }
}

static bool _ins_quit_process(const TcpConnectionPtr &conn,
    const std::string &source,
    const std::string &command,
    const std::vector<std::string> &args)
{
    std::lock_guard lock(nick_conn_mutex);

    nick_conn.erase(conn_session[conn].nickname);
    conn_session.erase(conn);

    // RETURN Closing Link: HOSTNAME (MSG)
}
}
