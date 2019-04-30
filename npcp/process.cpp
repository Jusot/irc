#include <mutex>
#include <string>
#include <unordered_map>

#include "../icarus/icarus/buffer.hpp"
#include "../icarus/icarus/tcpconnection.hpp"

#include "message.hpp"
#include "process.hpp"
#include "rplfuncs.hpp"

using namespace npcp;
using namespace icarus;

namespace
{
std::mutex nick_conn_mutex;
std::unordered_map<std::string, icarus::TcpConnectionPtr> nick_conn;
std::unordered_map<icarus::TcpConnectionPtr, Session> conn_session;
std::unordered_map<std::string, std::vector<std::string>> channels;

constexpr std::size_t cal_hash(const char *str)
{
    return (*str == '\0') ? 0 : ((cal_hash(str + 1) * 201314 + *str) % 5201314);
}

constexpr std::size_t operator""_hash(const char *str, std::size_t)
{
    return cal_hash(str);
}
}

bool Process::check_registered(const TcpConnectionPtr &conn)
{
    return conn_session.count(conn) && nick_conn.count(conn_session[conn].nickname);
}

void Process::on_message(const TcpConnectionPtr &conn, Buffer *buf)
{
    Message msg(buf->retrieve_all_as_string());

    auto hs = cal_hash(msg.command().c_str());

    switch (hs)
    {
    case "NICK"_hash:
        nick_process(conn, msg);
        break;

    case "USER"_hash:
        user_process(conn, msg);
        break;

#define RPL_WHEN_NOTREGISTERED if (!check_registered(conn)) \ 
        { \
            conn->send(Reply::err_notregistered()); \
            break; \
        }

    case "QUIT"_hash:
        RPL_WHEN_NOTREGISTERED;
        quit_process(conn, msg);
        break;

    case "PRIVMSG"_hash:
        RPL_WHEN_NOTREGISTERED;
        privmsg_process(conn, msg);
        break;

    case "PING"_hash:
        RPL_WHEN_NOTREGISTERED;
        ping_process(conn, msg);
        break;

    case "PONG"_hash:
        break;

    case "WHOIS"_hash:
        RPL_WHEN_NOTREGISTERED;
        break;

    case "MODE"_hash:
        RPL_WHEN_NOTREGISTERED;
        break;

    default:
        if (check_registered(conn))
            conn->send(Reply::err_unknowncommand(msg.command()));
        break;
    }
}

void Process::nick_process(const TcpConnectionPtr &conn, const Message &msg)
{
    std::lock_guard lock(nick_conn_mutex);

    if (msg.args().empty())
        conn->send(Reply::err_nonicknamegiven());
    else if (nick_conn.count(msg.args().front()))
        conn->send(Reply::err_nicknameinuse(msg.args().front()));
    else if (conn_session.count(conn))
    {
        if (nick_conn.count(conn_session[conn].nickname))
            nick_conn.erase(conn_session[conn].nickname);
        nick_conn[msg.args().front()] = conn;
        conn_session[conn] = { msg.args().front(), conn_session[conn].realname };

        conn->send(Reply::rpl_welcome(msg.source()));
        conn->send(Reply::rpl_yourhost("2"));
        conn->send(Reply::rpl_created());
        conn->send(Reply::rpl_myinfo("2", "ao", "mtov"));
    }
    else nick_conn[msg.args().front()] = nullptr;
}

void Process::user_process(const TcpConnectionPtr &conn, const Message &msg)
{
    std::lock_guard lock(nick_conn_mutex);

    if (conn_session.count(conn))
        conn->send(Reply::err_alreadyregistered());
    else if (msg.args().size() != 4)
        conn->send(Reply::err_needmoreparams(msg.command()));
    else
    {
        conn_session[conn] = { msg.args()[0], msg.args()[3] };
        if (nick_conn.count(msg.args()[0]))
        {
            conn->send(Reply::rpl_welcome(msg.source()));
            conn->send(Reply::rpl_yourhost("2"));
            conn->send(Reply::rpl_created());
            conn->send(Reply::rpl_myinfo("2", "ao", "mtov"));
        }
    }
}

void Process::quit_process(const TcpConnectionPtr &conn, const Message &msg)
{
    {
        std::lock_guard lock(nick_conn_mutex);
        nick_conn.erase(conn_session[conn].nickname);
        conn_session.erase(conn);
    }

    std::string quit_message = msg.args().empty() ? "" : msg.args().front();
    conn->send("Closing Link: HOSTNAME (" + quit_message + ")\r\n");
}

void Process::privmsg_process(const TcpConnectionPtr &conn, const Message &msg)
{
    if (msg.args().empty())
        conn->send(Reply::err_norecipient(msg.command()));
    else if (msg.args().size() == 1)
        conn->send(Reply::err_notexttosend());
    else if (!nick_conn.count(msg.args().front()))
        conn->send(Reply::err_nosuchnick(msg.args().front()));
    else
        nick_conn[msg.args().front()]->send(msg.raw());
}

void Process::ping_process(const TcpConnectionPtr &conn, const Message &msg)
{
    conn->send(Reply::rpl_pong(msg.hostname()));
}