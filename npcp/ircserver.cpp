#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>

#include "ircserver.hpp"
#include "rplfuncs.hpp"
#include "message.hpp"

#include "../icarus/icarus/buffer.hpp"
#include "../icarus/icarus/tcpserver.hpp"
#include "../icarus/icarus/tcpconnection.hpp"

namespace fs = std::filesystem;

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
    if (conn->connected() && !conn_session_.count(conn))
    {
        conn_session_[conn] = { Session::State::NONE, "*", "", "" };
    }
    else if (!conn->connected() && conn_session_.count(conn))
    {
        std::lock_guard lock(nick_conn_mutex_);
        if (conn_session_[conn].nickname != "*") 
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

            case ""_hash:
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

            case "OPER"_hash:
                RPL_WHEN_NOTREGISTERED;
                oper_process(conn, msg);
                break;

            case "MODE"_hash:
                RPL_WHEN_NOTREGISTERED;
                mode_process(conn, msg);
                break;

            case "JOIN"_hash:
                RPL_WHEN_NOTREGISTERED;
                join_process(conn, msg);
                break;

            default:
                if (check_registered(conn))
                    conn->send(reply::err_unknowncommand(
                        conn_session_.count(conn) ? conn_session_[conn].nickname : "*",
                        msg.command())
                    );
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
        conn->send(reply::err_norecipient(conn_session_[conn].nickname, msg.command()));
    else if (msg.args().size() == 1)
        conn->send(reply::err_notexttosend(conn_session_[conn].nickname));
    else if (!nick_conn_.count(msg.args()[0]))
        conn->send(reply::err_nosuchnick(conn_session_[conn].nickname, msg.args()[0]));
    else
        nick_conn_[msg.args()[0]]->send(reply::rpl_privmsg_or_notice(conn_session_[conn].nickname,
        conn_session_[conn].username,
        true,
        msg.args()[0], 
        msg.args()[1]));
}

void IrcServer::notice_process(const TcpConnectionPtr &conn, const Message &msg)
{
    if (msg.args().size() >= 2 && nick_conn_.count(msg.args().front()))
        nick_conn_[msg.args()[0]]->send(reply::rpl_privmsg_or_notice(conn_session_[conn].nickname,
        conn_session_[conn].username,
        false,
        msg.args()[0], 
        msg.args()[1]));
}

void IrcServer::ping_process(const TcpConnectionPtr &conn, const Message &msg)
{
    conn->send(reply::rpl_pong("jusot.com"));
}

void IrcServer::motd_process(const TcpConnectionPtr &conn, const Message &msg)
{
    const auto nick = conn_session_[conn].nickname;
    if (fs::is_regular_file("./motd.txt"))
    {
        std::ifstream fin("./motd.txt");
        conn->send(reply::rpl_motdstart(nick));

        std::string line;
        while (fin >> line) conn->send(reply::rpl_motd(nick, line));
        if (!fin.eof()) conn->send(reply::rpl_motd(nick, ""));

        conn->send(reply::rpl_endofmotd(nick));
    }
    else conn->send(reply::err_nomotd(nick));
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
        conn->send(reply::err_nosuchnick(conn_session_[conn].nickname, nick));
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

void IrcServer::oper_process(const TcpConnectionPtr& conn, const Message& msg)
{
    auto args = msg.args();
    if (args.size() < 2)
        conn->send(reply::err_needmoreparams(conn_session_[conn].nickname, "OPER"));
    else if (args[1] != "foobar") // password is foobar
        conn->send(reply::err_passwdmismatch(conn_session_[conn].nickname));
    else
        conn->send(reply::rpl_youareoper(conn_session_[conn].nickname));
}

void IrcServer::mode_process(const TcpConnectionPtr& conn, const Message& msg)
{
    auto args = msg.args();
    const auto nick = conn_session_[conn].nickname;
    const auto mode = args[1];
    if (args[0] != nick)
    {
        conn->send(reply::err_usersdontmatch(nick));
    }
    else if (mode[0] != '+' && mode[0] != '-')
    {
        conn->send(reply::err_usersdontmatch(nick));
    }
    else
    {
        switch (mode[1])
        {
            case 'o':
                if (mode[0] == '-')
                {
                    conn->send(":" + nick + " MODE " + nick + " :" + mode + "\r\n");
                }
                break;

            case 'a':
                break;


            default:
                conn->send(reply::err_umodeunknownflag(nick));
                break;
        }
    }

}

void IrcServer::join_process(const TcpConnectionPtr& conn, const Message& msg)
{
    const auto nick = conn_session_[conn].nickname, 
               user = conn_session_[conn].username;
    const auto args = msg.args();

    if (args.empty()) conn->send(reply::err_needmoreparams(nick, msg.command()));
    else if (!channels_.count(args[0]) ||
        std::find(channels_[args[0]].begin(), channels_[args[0]].end(), nick) == channels_[args[0]].end())
    {
        auto &nicks = channels_[args[0]];
        nicks.push_back(nick);
        
        auto replayed_join = reply::rpl_join(nick, user, args[0]);
        for (const auto &nick : nicks) nick_conn_[nick]->send(replayed_join);

        conn->send(reply::rpl_namreply(
            nick, 
            args[0], 
            std::vector<std::string>(nicks.begin(), nicks.end()) ));
        conn->send(reply::rpl_endofnames(nick, args[0]));
    }
}

} // namespace npcp