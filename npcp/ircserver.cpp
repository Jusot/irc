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

void IrcServer::on_message(const TcpConnectionPtr &conn, Buffer *buf)
{
    const char* crlf = buf->findCRLF();
    if (crlf == nullptr)
        return;
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
        conn->send(reply::err_notregistered()); \
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

void IrcServer::nick_process(const TcpConnectionPtr &conn, const Message &msg)
{
    std::lock_guard lock(nick_conn_mutex_);
    if (msg.args().empty())
        conn->send(reply::err_nonicknamegiven());
    else if (nick_conn_.count(msg.args().front()))
        conn->send(reply::err_nicknameinuse(msg.args().front()));
    else if (conn_session_.count(conn) && conn_session_[conn].state == Session::State::USER)
    {
        if (nick_conn_.count(conn_session_[conn].nickname))
            nick_conn_.erase(conn_session_[conn].nickname);
        nick_conn_[msg.args().front()] = conn;

        auto &session = conn_session_[conn];
        session = { Session::State::REGISTERED, msg.args().front(), session.username, session.realname };

        conn->send(reply::rpl_welcome(session.nickname,
            session.username,
            "jusot.com"));
        conn->send(reply::rpl_yourhost("2"));
        conn->send(reply::rpl_created());
        conn->send(reply::rpl_myinfo("2", "ao", "mtov"));
    }
    else
    {
        nick_conn_[msg.args().front()] = conn;
        conn_session_[conn] = {Session::State::NICK, msg.args().front(), "", ""};
    }
}

void IrcServer::user_process(const TcpConnectionPtr &conn, const Message &msg)
{
    std::lock_guard lock(nick_conn_mutex_);

    if (check_registered(conn))
        conn->send(reply::err_alreadyregistered());
    else if (msg.args().size() != 4)
        conn->send(reply::err_needmoreparams(msg.command()));
    else if (conn_session_.count(conn) && conn_session_[conn].state == Session::State::NICK)
    {
        auto &session = conn_session_[conn];
        session = { Session::State::REGISTERED, session.nickname, msg.args()[0], msg.args()[3] };
        nick_conn_[session.nickname] = conn;

        conn->send(reply::rpl_welcome(session.nickname,
            session.username,
            "jusot.com"));
        conn->send(reply::rpl_yourhost("2"));
        conn->send(reply::rpl_created());
        conn->send(reply::rpl_myinfo("2", "ao", "mtov"));
    }
    else
    {
        conn_session_[conn] = { Session::State::USER, "", msg.args()[0], msg.args()[3] };
    }
}

void IrcServer::quit_process(const TcpConnectionPtr &conn, const Message &msg)
{
    {
        std::lock_guard lock(nick_conn_mutex_);
        conn_session_[conn].state = Session::State::NICK;
        nick_conn_[conn_session_[conn].nickname].reset();
    }

    std::string quit_message = msg.args().empty() ? "" : msg.args().front();
    conn->send("Closing Link: HOSTNAME (" + quit_message + ")\r\n");
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
    conn->send(reply::err_nomotd());
}

void IrcServer::lusers_process(const TcpConnectionPtr& conn, const Message& msg)
{

}

void IrcServer::whois_process(const TcpConnectionPtr& conn, const Message& msg)
{

}
} // namespace npcp