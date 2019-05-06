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
    return reply + "\r\n";
}
} // namespace

namespace npcp
{
namespace reply
{
std::string rpl_pong(const std::string& server)
{
    return gen_reply({ _m_hostname,
        "PONG",
        server });
}

std::string rpl_welcome(const std::string& nick,
    const std::string &user,
    const std::string &host)
{
    return gen_reply({
        _m_hostname,
        "001",
        ":Welcome to the Internet Relay Network",
        nick + "!" + user + "@." + host
        });
}

std::string rpl_yourhost(const std::string & ver)
{
    return gen_reply({
        _m_hostname,
        "002",
        ":Your host is " + _hostname + ", running version " + ver
        });
}

std::string rpl_created()
{
    auto t = std::time(nullptr);
    return gen_reply({
        _m_hostname,
        "003",
        ":This server was created " + std::string(std::ctime(&t))
        });
}

std::string rpl_myinfo(const std::string & version,
    const std::string & avaliable_user_modes,
    const std::string & avaliable_channel_modes)
{
    return gen_reply({
        _m_hostname,
        "004",
        _m_hostname,
        version,
        avaliable_user_modes + "\n" + avaliable_channel_modes
        });
}

std::string err_nosuchnick(const std::string & nickname)
{
    return gen_reply({
        _m_hostname,
        "401",
        nickname,
        ":No such nick/channel"
        });
}

std::string err_norecipient(const std::string & command)
{
    return gen_reply({
        _m_hostname,
        "411",
        ":No recipient given",
        "(" + command + ")"
        });
}

std::string err_notexttosend()
{
    return gen_reply({
        _m_hostname,
        "412",
        ":No text to send"
        });
}

std::string err_unknowncommand(const std::string & command)
{
    return gen_reply({
        _m_hostname,
        "421",
        command,
        ":Unknown command"
        });
}

std::string err_nomotd()
{
    return gen_reply({
        _m_hostname,
        "422",
        ":MOTD File is missing"
        });
}

std::string err_nonicknamegiven()
{
    return gen_reply({
        _m_hostname,
        "431",
        ":No nickname given"
        });
}

std::string err_nicknameinuse(const std::string & nick)
{
    return gen_reply({
        _m_hostname,
        "432",
        nick,
        ":Nickname is already in use"
        });
}

std::string err_notregistered()
{
    return gen_reply({
        _m_hostname,
        "451",
        ":You have not registered"
        });
}

std::string err_needmoreparams(const std::string & command)
{
    return gen_reply({
        _m_hostname,
        "461",
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
} // namespace reply
} // namespace npcp