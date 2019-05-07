#include <string>

#include "ircserver.hpp"
#include "rplfuncs.hpp"
#include "message.hpp"

#include "../icarus/icarus/buffer.hpp"
#include "../icarus/icarus/tcpserver.hpp"
#include "../icarus/icarus/tcpconnection.hpp"

namespace
{
constexpr std::size_t cal_hash(const char* str)
{
    return (*str == '\0') ? 0 : ((cal_hash(str + 1) * 201314 + *str) % 5201314);
}

constexpr std::size_t operator""_hash(const char* str, std::size_t)
{
    return cal_hash(str);
}
} // namespace

namespace npcp
{
using namespace icarus;

IrcServer::IrcServer(EventLoop *loop, const InetAddress &listen_addr, std::string name)
  : server_(loop, listen_addr, std::move(name))
{
    server_.set_connection_callback([this] (const TcpConnectionPtr& conn) {
        this->on_connection(conn);
    });
    server_.set_message_callback([this] (const TcpConnectionPtr& conn, Buffer* buf) {
        this->on_message(conn, buf);
    });
}

void IrcServer::start()
{
    server_.start();
}

bool IrcServer::check_registered(const TcpConnectionPtr &conn)
{
    return conn_session_.count(conn) && conn_session_[conn].state == Session::State::REGISTERED;
}

void IrcServer::on_connection(const TcpConnectionPtr &conn)
{
    if (!conn->connected() && conn_session_.count(conn))
    {
        std::lock_guard lock(nick_conn_mutex_);
        nick_conn_.erase(conn_session_[conn].nickname);
        conn_session_.erase(conn);
    }
}

void IrcServer::on_message(const TcpConnectionPtr &conn, Buffer *buf)
{
    while (const char* crlf = buf->findCRLF())
    {
        Message msg(buf->retrieve_as_string(crlf - buf->peek() + 2));
        auto hs = cal_hash(msg.command().c_str());

        switch (hs)
        {
            case "NICK"_hash:
                nick_process(conn, msg);
                break;

            case "USER"_hash:
                user_process(conn, msg);
                break;

#define RPL_WHEN_NOTREGISTERED \
            if (!check_registered(conn)) \
            { \
                conn->send(reply::err_notregistered(conn_session_.count(conn) ? conn_session_[conn].nickname : "*")); \
                break; \
            }

            case "QUIT"_hash:
                quit_process(conn, msg);
                break;

            case "PRIVMSG"_hash:
                RPL_WHEN_NOTREGISTERED;
                privmsg_process(conn, msg);
                break;

            case "NOTICE"_hash:
                RPL_WHEN_NOTREGISTERED;
                notice_process(conn, msg);
                break;

            case "PING"_hash:
                RPL_WHEN_NOTREGISTERED;
                ping_process(conn, msg);
                break;

            case "PONG"_hash:
                break;

            case "MOTD"_hash:
                RPL_WHEN_NOTREGISTERED;
                motd_process(conn, msg);
                break;

            case "LUSERS"_hash:
                RPL_WHEN_NOTREGISTERED;
                lusers_process(conn, msg);
                break;

            case "WHOIS"_hash:
                RPL_WHEN_NOTREGISTERED;
                whois_process(conn, msg);
                break;

            case "MODE"_hash:
                RPL_WHEN_NOTREGISTERED;
                break;

            default:
                if (check_registered(conn))
                    conn->send(reply::err_unknowncommand(msg.command()));
                break;
        }
    }
}

void IrcServer::nick_process(const TcpConnectionPtr &conn, const Message &msg)
{
    std::lock_guard lock(nick_conn_mutex_);
    if (msg.args().empty())
        conn->send(reply::err_nonicknamegiven());
    else if (nick_conn_.count(msg.args().front()))
        conn->send(reply::err_nicknameinuse(msg.args().front()));
    else if (conn_session_.count(conn) && conn_session_[conn].state == Session::State::USER)
    {
        auto &session = conn_session_[conn];
        const auto nick = msg.args().front();

        if (nick_conn_.count(session.nickname))
            nick_conn_.erase(session.nickname);
        nick_conn_[nick] = conn;

        session = {
            Session::State::REGISTERED,
            nick,
            session.username,
            session.realname
        };

        conn->send(reply::rpl_welcome(
            session.nickname,
            session.username,
            "jusot.com") + 
            reply::rpl_yourhost(nick, "2") + 
            reply::rpl_created(nick) +
            reply::rpl_myinfo(nick, "2", "ao", "mtov")
        );
        lusers_process(conn, msg);
        motd_process(conn, msg);
    }
    else
    {
        const auto nick = msg.args().front();

        nick_conn_[nick] = conn;
        conn_session_[conn] = {Session::State::NICK, nick, "", ""};
    }
}

void IrcServer::user_process(const TcpConnectionPtr &conn, const Message &msg)
{
    std::lock_guard lock(nick_conn_mutex_);

    if (check_registered(conn))
        conn->send(reply::err_alreadyregistered());
    else if (msg.args().size() != 4)
        conn->send(reply::err_needmoreparams(conn_session_.count(conn) ? conn_session_[conn].nickname : "*", msg.command()));
    else if (conn_session_.count(conn) && conn_session_[conn].state == Session::State::NICK)
    {
        auto& session = conn_session_[conn];
        session = {
            Session::State::REGISTERED,
            session.nickname,
            msg.args()[0],
            msg.args()[3]
        };

        conn->send(reply::rpl_welcome(session.nickname,
            session.username,
            "jusot.com" ) + 
            reply::rpl_yourhost(session.nickname, "2") + 
            reply::rpl_created(session.nickname) +
            reply::rpl_myinfo(session.nickname, "2", "ao", "mtov")
        );

        lusers_process(conn, msg);
        motd_process(conn, msg);
    }
    else
    {
        conn_session_[conn] = { Session::State::USER, "*", msg.args()[0], msg.args()[3] };
    }
}

void IrcServer::quit_process(const TcpConnectionPtr &conn, const Message &msg)
{
    {
        std::lock_guard lock(nick_conn_mutex_);
        nick_conn_.erase(conn_session_[conn].nickname);
        conn_session_.erase(conn);
    }

    std::string quit_message = msg.args().empty() ? "Client Quit" : msg.args().front();
    conn->send(":jusot.com ERROR :Closing Link: jusot.com (" + quit_message + ")\r\n");
    conn->get_loop()->queue_in_loop([conn] () {
        conn->shutdown();
    });
}

void IrcServer::privmsg_process(const TcpConnectionPtr &conn, const Message &msg)
{
    if (msg.args().empty())
        conn->send(reply::err_norecipient(msg.command()));
    else if (msg.args().size() == 1)
        conn->send(reply::err_notexttosend());
    else if (!nick_conn_.count(msg.args().front()))
        conn->send(reply::err_nosuchnick(msg.args().front()));
    else
        nick_conn_[msg.args().front()]->send(msg.raw());
}

void IrcServer::notice_process(const TcpConnectionPtr &conn, const Message &msg)
{
    if (msg.args().size() >= 2 && nick_conn_.count(msg.args().front()))
        nick_conn_[msg.args().front()]->send(msg.raw());
}

void IrcServer::ping_process(const TcpConnectionPtr &conn, const Message &msg)
{
    conn->send(reply::rpl_pong("jusot.com"));
}

void IrcServer::motd_process(const TcpConnectionPtr &conn, const Message &msg)
{
    // check 'motd.txt' exits or not
    conn->send(reply::err_nomotd(conn_session_[conn].nickname));
}

void IrcServer::lusers_process(const TcpConnectionPtr& conn, const Message& msg)
{
    int users = 0;
    int unknowns = 0;
    for (const auto& pair: conn_session_)
    {
        if (pair.second.state == Session::State::REGISTERED)
            ++users;
        else
            ++unknowns;
    }

    const auto nick = conn_session_[conn].nickname;

    conn->send(
        reply::rpl_luserclient(nick, users, 0, 1) +
        reply::rpl_luserop(nick, 0) + 
        reply::rpl_luserunknown(nick, unknowns) +
        reply::rpl_luserchannels(nick, channels_.size()) +
        reply::rpl_luserme(nick, users + unknowns, 1)
    );
}

void IrcServer::whois_process(const TcpConnectionPtr& conn, const Message& msg)
{
    auto args = msg.args();
    if (args.size() != 1)
        return;

    std::string nick = args[0];

//    const auto& nick = conn_session_[conn].nickname;
    if (nick_conn_.find(nick) == nick_conn_.end())
    {
        conn->send(reply::err_nosuchnick(nick));
    }
    else
    {
        auto session = conn_session_[nick_conn_[nick]];
        std::string user = session.username;
        std::string realname = session.realname;
        conn->send(reply::rpl_whoisuser(nick, user, realname));
        conn->send(reply::rpl_whoisserver(nick));
        conn->send(reply::rpl_endofwhois(nick));
    }

}
} // namespace npcp