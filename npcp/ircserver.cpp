#include <set>
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

constexpr uint32_t kChannelMode_m = 0b1;
constexpr uint32_t kChannelMode_t = 0b10;
constexpr uint32_t kChannelMode_v = 0x100;

std::string channel_mode_to_string(uint32_t mode)
{
    std::string str_mode("+");
    if (mode & kChannelMode_m)
        str_mode.push_back('m');
    if (mode & kChannelMode_t)
        str_mode.push_back('t');
    if (mode & kChannelMode_v)
        str_mode.push_back('v');
    return str_mode;
}

inline void set_channel_mode(uint32_t& mode, uint32_t mask)
{
    mode |= mask;
}

inline void unset_channel_mode(uint32_t& mode, uint32_t mask)
{
    mode &= ~mask;
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
    server_.set_thread_num(10);
}

void IrcServer::start()
{
    server_.start();
}

bool IrcServer::check_registered(const TcpConnectionPtr &conn)
{
    return conn_session_.count(conn) &&
        (conn_session_[conn].state == Session::State::REGISTERED || conn_session_[conn].state == Session::State::AWAY);
}

bool IrcServer::check_in_channel(const TcpConnectionPtr &conn, const std::string &channel)
{
    if (!channels_.count(channel)) return false;
    const auto &nicks = channels_[channel].users;
    return std::find(nicks.begin(), nicks.end(), conn_session_[conn].nickname) != nicks.end();
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

            case "PART"_hash:
                RPL_WHEN_NOTREGISTERED;
                part_process(conn, msg);
                break;
            
            case "TOPIC"_hash:
                RPL_WHEN_NOTREGISTERED;
                topic_process(conn, msg);
                break;

            case "AWAY"_hash:
                RPL_WHEN_NOTREGISTERED;
                away_process(conn, msg);
                break;

            case "NAMES"_hash:
                RPL_WHEN_NOTREGISTERED;
                names_process(conn, msg);
                break;

            case "LIST"_hash:
                RPL_WHEN_NOTREGISTERED;
                list_process(conn, msg);
                break;

            case "WHO"_hash:
                RPL_WHEN_NOTREGISTERED;
                who_process(conn, msg);
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
    const auto nick = conn_session_[conn].nickname,
               user = conn_session_[conn].username;
    const auto args = msg.args();

    if (args.empty())
        conn->send(reply::err_norecipient(nick, msg.command()));
    else if (args.size() == 1)
        conn->send(reply::err_notexttosend(nick));
    else if (!nick_conn_.count(args[0]) && !channels_.count(args[0]))
        conn->send(reply::err_nosuchnick(nick, args[0]));
    else if (nick_conn_.count(args[0]))
    {
        if (conn_session_[nick_conn_[args[0]]].state == Session::State::AWAY)
        {
            conn->send(reply::rpl_away(nick, args[0], nick_awaymsg_[args[0]]));
        }
        else
        {
            nick_conn_[args[0]]->send(reply::rpl_privmsg_or_notice(
                nick, user, true, args[0], args[1]));
        }
    }
    else if (!check_in_channel(conn, args[0]))
        conn->send(reply::err_cannotsendtochan(nick, args[0]));
    else
    {
        const auto &nicks = channels_[args[0]].users;
        auto rpl = reply::rpl_privmsg_or_notice(
            nick, user, true, args[0], args[1]);
        for (const auto &peer : nicks) if (peer != nick)
            nick_conn_[peer]->send(rpl);
    }
}

void IrcServer::notice_process(const TcpConnectionPtr &conn, const Message &msg)
{
    const auto nick = conn_session_[conn].nickname,
               user = conn_session_[conn].username;
    const auto args = msg.args();

    if (args.size() < 2) return;

    if (nick_conn_.count(args[0]))
        nick_conn_[args[0]]->send(reply::rpl_privmsg_or_notice(
            nick, user, false, args[0], args[1]));
    else if (check_in_channel(conn, args[0]))
    {
        const auto &nicks = channels_[args[0]].users;
        auto rpl = reply::rpl_privmsg_or_notice(
            nick, user, true, args[0], args[1]);
        for (const auto &peer : nicks) if (peer != nick)
            nick_conn_[peer]->send(rpl);
    }
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

    if (args.empty())
    {
        conn->send(reply::err_needmoreparams(nick, "MODE"));
        return;
    }

    if (args[0][0] != '#')  // user mode
    {
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
    else  // channel mode
    {
        const auto& channel = args[0];
        if (channels_.find(channel) == channels_.end())
        {
            conn->send(reply::err_nosuchchannel(nick, channel));
        }
        else if (args.size() == 1)
        {
            conn->send(reply::rpl_channelmodeis(nick, channel, channel_mode_to_string(channels_[channel].mode)));
        }
        else if (args.size() == 2)
        {
            const auto &mode = args[1];
            if (mode[0] != '+' && mode[0] != '-')
            {
                conn->send(reply::err_unknownmode(nick, mode[1], channel));
            }
            else
            {

#define PROCESS_MODE(M) \
    if (mode[0] == '+') \
    { \
        set_channel_mode(channels_[channel].mode, kChannelMode_##M); \
        if (channels_[channel].operators.find(nick) != channels_[channel].operators.end()) \
        { \
            for (const auto& user: channels_[channel].users) \
            { \
                nick_conn_[user]->send(":" + nick + "!" + conn_session_[nick_conn_[user]].username + "@jusot.com MODE " + channel + " " + mode + "\r\n"); \
            } \
        } \
        else \
        { \
            conn->send(reply::err_chanoprivsneeded(nick, channel)); \
        } \
    } \
    else if (mode[0] == '-') \
    { \
        unset_channel_mode(channels_[channel].mode, kChannelMode_##M); \
        conn->send(":" + nick + "!" + conn_session_[conn].username + "@jusot.com MODE " + channel + " " + mode + "\r\n"); \
    }

                switch (mode[1])
                {
                    case 'm':
                        PROCESS_MODE(m);
                        break;

                    case 't':
                        PROCESS_MODE(t);
                        break;

//                    case 'v':
//                        break;

                    default:
                        conn->send(reply::err_unknownmode(nick, mode[1], channel));
                        break;
                }
#undef PROCESS_MODE
            }
        }
        else if (args.size() == 3)
        {
            const auto& mode = args[1];
            const auto& nick_mode = args[2];
            if (mode[0] != '+' && mode[0] != '-')
            {
                conn->send(reply::err_unknownmode(nick, mode[1], channel));
            }
            else
            {
                switch (mode[1])
                {
                    case 'v':
                        if (mode[0] == '+')
                        {
                            if (channels_[channel].operators.find(nick) != channels_[channel].operators.end())
                            {
                                const auto& users = channels_[channel].users;
                                if (std::find(users.begin(), users.end(), nick_mode) != users.end())
                                {
                                    channels_[channel].voices.insert(nick_mode);
                                    for (const auto& user: channels_[channel].users)
                                    {
                                        std::stringstream reply;
                                        reply << ":" << nick << "!" << conn_session_[nick_conn_[user]].username
                                              << "@jusot.com MODE " << channel << " " << mode << " " << nick_mode << "\r\n";
                                        nick_conn_[user]->send(reply.str());
                                    }
                                }
                                else
                                {
                                    conn->send(reply::err_usernotinchannel(nick, nick_mode, channel));
                                }
                            }
                            else
                            {
                                conn->send(reply::err_chanoprivsneeded(nick, channel));
                            }
                        }
                        else
                        {
                            if (channels_[channel].operators.find(nick) != channels_[channel].operators.end())
                            {
                                const auto& users = channels_[channel].users;
                                if (std::find(users.begin(), users.end(), nick_mode) != users.end())
                                {
                                    channels_[channel].voices.erase(nick_mode);
                                    for (const auto& user: channels_[channel].users)
                                    {
                                        std::stringstream reply;
                                        reply << ":" << nick << "!" << conn_session_[nick_conn_[user]].username
                                              << "@jusot.com MODE " << channel << " " << mode << " " << nick_mode << "\r\n";
                                        nick_conn_[user]->send(reply.str());
                                    }
                                }
                                else
                                {
                                    conn->send(reply::err_usernotinchannel(nick, nick_mode, channel));
                                }
                            }
                            else
                            {
                                conn->send(reply::err_chanoprivsneeded(nick, channel));
                            }
                        }

                        break;


                    case 'o':
                        if (mode[0] == '+')
                        {
                            if (channels_[channel].operators.find(nick) != channels_[channel].operators.end())
                            {
                                const auto& users = channels_[channel].users;
                                if (std::find(users.begin(), users.end(), nick_mode) != users.end())
                                {
                                    channels_[channel].operators.insert(nick_mode);
                                    for (const auto& user: channels_[channel].users)
                                    {
                                        std::stringstream reply;
                                        reply << ":" << nick << "!" << conn_session_[nick_conn_[user]].username
                                              << "@jusot.com MODE " << channel << " " << mode << " " << nick_mode << "\r\n";
                                        nick_conn_[user]->send(reply.str());
                                    }
                                }
                                else
                                {
                                    conn->send(reply::err_usernotinchannel(nick, nick_mode, channel));
                                }
                            }
                            else
                            {
                                conn->send(reply::err_chanoprivsneeded(nick, channel));
                            }
                        }
                        else
                        {
                            if (channels_[channel].operators.find(nick) != channels_[channel].operators.end())
                            {
                                const auto& users = channels_[channel].users;
                                if (std::find(users.begin(), users.end(), nick_mode) != users.end())
                                {
                                    channels_[channel].operators.erase(nick_mode);
                                    for (const auto& user: channels_[channel].users)
                                    {
                                        std::stringstream reply;
                                        reply << ":" << nick << "!" << conn_session_[nick_conn_[user]].username
                                              << "@jusot.com MODE " << channel << " " << mode << " " << nick_mode << "\r\n";
                                        nick_conn_[user]->send(reply.str());
                                    }
                                }
                                else
                                {
                                    conn->send(reply::err_usernotinchannel(nick, nick_mode, channel));
                                }
                            }
                            else
                            {
                                conn->send(reply::err_chanoprivsneeded(nick, channel));
                            }
                        }
                        break;

                    default:
                        conn->send(reply::err_unknownmode(nick, mode[1], channel));
                        break;
                }
            }
        }
    }
}

void IrcServer::join_process(const TcpConnectionPtr& conn, const Message& msg)
{
    const auto nick = conn_session_[conn].nickname, 
               user = conn_session_[conn].username;
    const auto args = msg.args();

    if (args.empty()) conn->send(reply::err_needmoreparams(nick, msg.command()));
    else if (!channels_.count(args[0]) || !check_in_channel(conn, args[0]))
    {
        auto &chinfo = channels_[args[0]];
        auto &nicks = chinfo.users;
        if (nicks.empty()) chinfo.operators.insert(nick);
        nicks.push_back(nick);

        auto replayed_join = reply::rpl_join(nick, user, args[0]);
        for (const auto &peer : nicks) nick_conn_[peer]->send(replayed_join);

        if (!channels_[args[0]].topic.empty())
            conn->send(reply::rpl_topic(nick, args[0], channels_[args[0]].topic));

        auto users = chinfo.users;
        for (auto &user : users)
        {
            if (chinfo.operators.count(user))
                user = "@" + user;
            if (chinfo.voices.count(user))
                user = "+" + user;
        }

        conn->send(reply::rpl_namreply(
            nick, args[0], users ));
        conn->send(reply::rpl_endofnames(nick, args[0]));
    }
}

void IrcServer::part_process(const TcpConnectionPtr &conn, const Message &msg)
{
    const auto nick = conn_session_[conn].nickname, 
               user = conn_session_[conn].username;
    const auto args = msg.args();

    if (args.empty()) conn->send(reply::err_needmoreparams(nick, msg.command()));
    else
    {
        const auto channel = args[0],
                   message = args.size() == 1 ? "" : args[1];
        if (!channels_.count(channel)) conn->send(reply::err_nosuchchannel(nick, channel));
        else if (!check_in_channel(conn, channel)) conn->send(reply::err_notonchannel(nick, channel));
        else
        {
            auto rpl = reply::rpl_part(nick, user, channel, message);

            auto &users = channels_[channel].users;
            for (const auto &peer : users) nick_conn_[peer]->send(rpl);

            auto pos = std::find(users.begin(), users.end(), nick);
            users.erase(pos);
            if (users.empty()) channels_.erase(channel);
        }
    }
}

void IrcServer::topic_process(const TcpConnectionPtr &conn, const Message &msg)
{
    const auto nick = conn_session_[conn].nickname, 
               user = conn_session_[conn].username;
    const auto args = msg.args();
    
    if (args.empty()) conn->send(reply::err_needmoreparams(nick, msg.command()));
    else
    {
        const auto channel = args[0],
                   topic   = args.size() == 1 ? "" : args[1];
        if (!check_in_channel(conn, channel)) conn->send(reply::err_notonchannel(nick, channel));
        else if (args.size() == 2)
        {
            auto &chinfo = channels_[channel];
            chinfo.topic = topic;
            auto rpl = reply::rpl_relayed_topic(nick, user, channel, topic);

            auto &users = channels_[channel].users;
            for (const auto &peer : users) nick_conn_[peer]->send(rpl);
        }
        else if (channels_[args[0]].topic.empty())
        {
            conn->send(reply::rpl_notopic(nick, args[0]));
        }
        else
        {
            conn->send(reply::rpl_topic(nick, args[0], channels_[args[0]].topic));
        }
    }
}

void IrcServer::away_process(const TcpConnectionPtr &conn, const Message &msg)
{
    auto &session = conn_session_[conn];

    if (!msg.args().empty())
    {
        session.state = Session::State::AWAY;
        nick_awaymsg_[session.nickname] = msg.args()[0];

        conn->send(reply::rpl_nowaway(session.nickname));
    }
    else
    {
        session.state = Session::State::REGISTERED;
        nick_awaymsg_.erase(session.nickname);

        conn->send(reply::rpl_unaway(session.nickname));
    }
}

void IrcServer::names_process(const icarus::TcpConnectionPtr &conn, const Message &msg)
{
    const auto nick = conn_session_[conn].nickname;

    if (msg.args().empty())
    {
        std::set<std::string> allnicks;
        for (const auto & nick_c : nick_conn_) allnicks.insert(nick_c.first);

        for (const auto & c_chinfo : channels_)
        {
            const auto & chinfo = c_chinfo.second;
            if (!chinfo.users.empty())
            {
                auto users = chinfo.users;
                for (auto &user : users)
                {
                    if (chinfo.operators.count(user))
                        user = "@" + user;
                    if (chinfo.voices.count(user))
                        user = "+" + user;
                }
                conn->send(reply::rpl_namreply(
                    nick, c_chinfo.first, users
                ));
            }
            for (const auto & nickname : chinfo.users)
                if (allnicks.count(nickname)) allnicks.erase(nickname);
        }
        if (!allnicks.empty()) conn->send(reply::rpl_namreply(
            nick, "*", std::vector<std::string>(allnicks.begin(), allnicks.end())
        ));
        
        conn->send(reply::rpl_endofnames(nick, "*"));
    }
    else
    {
        const auto channel = msg.args()[0];
        if (channels_.count(channel))
        {
            auto &chinfo = channels_[channel];
            auto users = channels_[channel].users;
            for (auto &user : users)
            {
                if (chinfo.operators.count(user))
                    user = "@" + user;
                if (chinfo.voices.count(user))
                    user = "+" + user;
            }
            conn->send(reply::rpl_namreply(
                nick, channel, users ));
        }
        conn->send(reply::rpl_endofnames(nick, channel));
    }
}

void IrcServer::list_process(const icarus::TcpConnectionPtr &conn, const Message &msg)
{
    const auto nick = conn_session_[conn].nickname;
    const auto args = msg.args();
    if (args.empty())
    {
        for (const auto &c_chinfo : channels_)
        {
            const auto &chinfo = c_chinfo.second;
            conn->send(reply::rpl_list(nick, c_chinfo.first, chinfo.users.size(), chinfo.topic));
        }
    }
    else
    {
        const auto &channel = args[0];
        const auto &chinfo = channels_[channel];

        conn->send(reply::rpl_list(nick, channel, chinfo.users.size(), chinfo.topic));
    }
    conn->send(reply::rpl_listend(nick));
}

void IrcServer::who_process(const icarus::TcpConnectionPtr &conn, const Message &msg)
{

}

} // namespace npcp