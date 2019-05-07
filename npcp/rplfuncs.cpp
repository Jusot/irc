#include <ctime>
#include <vector>
#include <cassert>
#include <string>
#include <sstream>

#include "rplfuncs.hpp"

namespace
{
const std::string _m_hostname(":jusot.com");
const std::string _hostname("jusot.com");

std::string gen_reply(const std::vector<std::string> &args)
{
    std::string reply;
    for (const auto& arg : args) reply.append(arg + ' ');
    reply.pop_back();
    return ((reply.size() > 510) ? reply.substr(0, 510) : reply) + "\r\n";
}
} // namespace

namespace npcp
{
namespace reply
{
std::string rpl_pong(const std::string& server)
{
    return gen_reply({
        _m_hostname,
        "PONG",
        server
    });
}

std::string rpl_privmsg_or_notice(const std::string& nick,
    const std::string& user,
    bool is_privmsg,
    const std::string& target,
    const std::string& msg)
{
    return gen_reply({
        ":" + nick + "!" + user + "@jusot.com",
        is_privmsg ? "PRIVMSG" : "NOTICE",
        target,
        ":" + msg
    });
}

std::string rpl_join(const std::string& nick,
    const std::string& user,
    const std::string& channel)
{
    return gen_reply({
        ":" + nick + "!" + user + "@jusot.com",
        "JOIN",
        channel
    });
}

std::string rpl_welcome(const std::string& nick,
    const std::string &user,
    const std::string &host)
{
    return gen_reply({
        _m_hostname,
        "001",
        nick,
        ":Welcome to the Internet Relay Network",
        nick + "!" + user + "@." + host
        });
}

std::string rpl_yourhost(const std::string & nick,
    const std::string &ver)
{
    return gen_reply({
        _m_hostname,
        "002",
        nick,
        ":Your host is " + _hostname + ", running version " + ver
        });
}

std::string rpl_created(const std::string & nick)
{
    // auto t = std::time(nullptr);
    return gen_reply({
        _m_hostname,
        "003",
        nick,
        ":This server was created " // + std::string(std::ctime(&t))
        });
}

std::string rpl_myinfo(const std::string & nick,
    const std::string & version,
    const std::string & avaliable_user_modes,
    const std::string & avaliable_channel_modes)
{
    return gen_reply({
        _m_hostname,
        "004",
        nick,
        _hostname,
        version,
        avaliable_user_modes,
        avaliable_channel_modes
        });
}

std::string rpl_luserclient(const std::string &nick,
    int users_cnt, int services_cnt, int servers_cnt)
{
    return gen_reply({
        _m_hostname,
        "251",
        nick,
        ":There are " + std::to_string(users_cnt) + " users",
        "and " + std::to_string(services_cnt) + " services",
        "on " + std::to_string(servers_cnt) + " servers"
        });
}

std::string rpl_luserop(const std::string &nick, int opers_cnt)
{
    return gen_reply({
        _m_hostname,
        "252",
        nick,
        std::to_string(opers_cnt),
        ":operator(s) online"
        });
}

std::string rpl_luserunknown(const std::string &nick, int unknown_connections_cnt)
{
    return gen_reply({
        _m_hostname,
        "253",
        nick,
        std::to_string(unknown_connections_cnt),
        ":unknown connection(s)"
        });
}

std::string rpl_luserchannels(const std::string &nick, int channels_cnt)
{
    return gen_reply({
        _m_hostname,
        "254",
        nick,
        std::to_string(channels_cnt),
        ":channels formed"
        });
}

std::string rpl_luserme(const std::string &nick, int clients_cnt, int servers_cnt)
{
    return gen_reply({
        _m_hostname,
        "255",
        nick,
        ":I have " + std::to_string(clients_cnt) + " clients",
        "and " + std::to_string(servers_cnt) + " servers"
        });
}

std::string rpl_whoisuser(const std::string& nick,          // 311
                          const std::string& user,
                          const std::string& realname)
{
    return gen_reply({
        _m_hostname,
        "311",
        nick,
        nick,
        user,
        _hostname,
        "*",
        ":" + realname
    });
}

std::string rpl_whoisserver(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "312",
        nick,
        nick,
        _hostname,
        ":server info"
    });
}

std::string rpl_endofwhois(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "318",
        nick,
        nick,
        ":End of WHOIS list"
    });
}

std::string rpl_channelmodeis(const std::string& nick,
                              const std::string& channel,
                              const std::string& mode)
{
    return gen_reply({
        _m_hostname,
        "324",
        nick,
        channel,
        mode
    });
}

std::string rpl_namreply(const std::string& nick,
    const std::string& channel,
    const std::vector<std::string>& nicks)
{
    std::string tailing = ":";
    for (const auto &nick : nicks) tailing += nick + " ";
    tailing.pop_back();

    return gen_reply({
        _m_hostname,
        "353",
        nick, "=", channel,
        tailing
    });
}

std::string rpl_endofnames(const std::string& nick,
    const std::string& channel)
{
    return gen_reply({
        _m_hostname,
        "366",
        nick,
        channel,
        ":End of NAMESN list"
    });
}

std::string rpl_motd(const std::string& nick,
    const std::string& line)
{
    return gen_reply({
        _m_hostname,
        "372",
        nick,
        ":- " + line
    });
}

std::string rpl_motdstart(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "375",
        nick,
        ":- .* Message of the day - "
    });
}

std::string rpl_endofmotd(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "376",
        nick,
        ":End of MOTD command"
    });
}

std::string rpl_youareoper(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "381",
        nick,
        ":You are now an IRC operator"
    });
}


std::string err_nosuchnick(const std::string & nick,
                               const std::string & target)
{
    return gen_reply({
        _m_hostname,
        "401",
        nick,
        target,
        ":No such nick/channel"
        });
}

std::string err_nosuchchannel(const std::string& nick,
                              const std::string& channel)
{
    return gen_reply({
        _m_hostname,
        "403",
        nick,
        channel,
        ":No such channel"
    });
}

std::string err_cannotsendtochan(const std::string& nick,
    const std::string& channel)
{
    return gen_reply({
        _m_hostname,
        "404",
        nick,
        channel,
        ":Cannot send to channel"
    });
}

std::string err_norecipient(const std::string & nick,
    const std::string & command)
{
    return gen_reply({
        _m_hostname,
        "411",
        nick,
        ":No recipient given",
        "(" + command + ")"
        });
}

std::string err_notexttosend(const std::string & nick)
{
    return gen_reply({
        _m_hostname,
        "412",
        nick,
        ":No text to send"
        });
}

std::string err_unknowncommand(const std::string & nick,
    const std::string & command)
{
    return gen_reply({
        _m_hostname,
        "421",
        nick,
        command,
        ":Unknown command"
        });
}

std::string err_nomotd(const std::string &nick)
{
    return gen_reply({
        _m_hostname,
        "422",
        nick,
        ":MOTD File is missing"
        });
}

std::string err_nonicknamegiven()
{
    return gen_reply({
        _m_hostname,
        "431",
        "*",
        ":No nickname given"
        });
}

std::string err_nicknameinuse(const std::string & nick)
{
    return gen_reply({
        _m_hostname,
        "433",
        "*",
        nick,
        ":Nickname is already in use"
        });
}

std::string err_notregistered(const std::string & nick)
{
    return gen_reply({
        _m_hostname,
        "451",
        nick,
        ":You have not registered"
        });
}

std::string err_needmoreparams(const std::string & nick,
    const std::string & command)
{
    return gen_reply({
        _m_hostname,
        "461",
        nick,
        command,
        ":Not enough parameters"
        });
}

std::string err_alreadyregistered()
{
    return gen_reply({
        _m_hostname,
        "462",
        ":Unauthorized command (already registered)"
        });
}

std::string err_passwdmismatch(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "464",
        nick,
        ":Password incorrect"
    });
}

std::string err_umodeunknownflag(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "501",
        nick,
        ":Unknown MODE flag"
    });
}

std::string err_usersdontmatch(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "502",
        nick,
        ":Cannot change mode for other users"
    });
}

} // namespace reply
} // namespace npcp