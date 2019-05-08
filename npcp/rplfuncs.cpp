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

std::string rpl_part(const std::string& nick,
    const std::string& user,
    const std::string& channel,
    const std::string& message)
{
    if (message.empty()) return gen_reply({
        ":" + nick + "!" + user + "@jusot.com",
        "PART",
        channel
    });
    else return gen_reply({
        ":" + nick + "!" + user + "@jusot.com",
        "PART",
        channel,
        ":" + message
    });
}

std::string rpl_relayed_topic(const std::string& nick,
    const std::string& user,
    const std::string& channel,
    const std::string& topic)
{
    return gen_reply({
        ":" + nick + "!" + user + "@jusot.com",
        "TOPIC",
        channel,
        ":" + topic
    });
}

std::string rpl_relayed_nick(const std::string& nick,
    const std::string& user,
    const std::string& newnick)
{
    return gen_reply({
        ":" + nick + "!" + user + "@jusot.com",
        "NICK",
        ":" + newnick
    });
}

std::string rpl_relayed_quit(const std::string& nick,
    const std::string& user,
    const std::string& message)
{
    return gen_reply({
        ":" + nick + "!" + user + "@jusot.com",
        "QUIT",
        ":" + message
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

std::string rpl_away(const std::string& nick,
    const std::string& peer,
    const std::string& awaymsg)
{
    return gen_reply({
        _m_hostname,
        "301",
        nick,
        peer,
        ":" + awaymsg
    });
}

std::string rpl_unaway(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "305",
        nick,
        ":You are no longer marked as being away"
    });
}

std::string rpl_nowaway(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "306",
        nick,
        ":You have been marked as being away"
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

std::string rpl_endofwho(const std::string& nick,
    const std::string& name)
{
    return gen_reply({
        _m_hostname,
        "315",
        nick,
        name,
        ":End of WHO list"
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

std::string rpl_whoischannels(const std::string& nick,
                              const std::string& channels)
{
    return gen_reply({
        _m_hostname,
        "319",
        nick,
        nick,
        ":" + channels
    });
}

std::string rpl_list(const std::string& nick,
    const std::string& channel,
    int visable_num, 
    const std::string& topic)
{
    return gen_reply({
        _m_hostname,
        "322",
        nick,
        channel,
        std::to_string(visable_num),
        ":" + topic
    });
}

std::string rpl_listend(const std::string& nick)
{
    return gen_reply({
        _m_hostname,
        "323",
        nick,
        ":End of LIST"
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

std::string rpl_notopic(const std::string&nick,
    const std::string& channel)
{
    return gen_reply({
        _m_hostname,
        "331",
        nick,
        channel,
        ":No topic is set"
    });
}

std::string rpl_topic(const std::string&nick,
    const std::string& channel,
    const std::string& topic)
{
    return gen_reply({
        _m_hostname,
        "332",
        nick,
        channel,
        ":" + topic
    });
}

std::string rpl_whoreply(const std::string& nick,
    const std::string& channel,
    const std::string& user,
    const std::string& host,
    const std::string& server,
    const std::string& peernick,
    const std::string& flags,
    const std::string& realname)
{
    return gen_reply({
        _m_hostname,
        "352",
        nick,
        channel,
        user,
        host,
        server,
        peernick,
        flags,
        ":0",
        realname
    });
}

std::string rpl_namreply(const std::string& nick,
    const std::string& channel,
    const std::vector<std::string>& nicks)
{
    std::string tailing = ":";
    for (const auto &nick : nicks) tailing += nick + " ";
    tailing.pop_back();

    if (channel == "*") return gen_reply({
        _m_hostname,
        "353",
        nick,
        channel,
        channel,
        tailing
    });
    else return gen_reply({
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

std::string err_usernotinchannel(
        const std::string& nick,
        const std::string& nick_mode,
        const std::string& channel)
{
    return gen_reply({
        _m_hostname,
        "441",
        nick,
        nick_mode,
        channel,
        ":They aren't on that channel"
    });
}

std::string err_notonchannel(const std::string& nick,
    const std::string& channel)
{
    return gen_reply({
        _m_hostname,
        "442",
        nick,
        channel,
        ":You're not on that channel"
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

std::string err_unknownmode(const std::string& nick,
                            char mode,
                            const std::string& channel)
{
    std::string tmp(nick);
    tmp.push_back(' ');
    tmp.push_back(mode);
    return gen_reply({
        _m_hostname,
        "472",
        tmp,
        ":is unknown mode char to me for",
        channel
    });
}

std::string err_chanoprivsneeded(const std::string& nick,
                                 const std::string& channel)
{
    return gen_reply({
        _m_hostname,
        "482",
        nick,
        channel,
        ":You're not channel operator"
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