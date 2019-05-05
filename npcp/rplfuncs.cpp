#include <ctime>
#include <cassert>
#include <string>
#include <sstream>

#include "rplfuncs.hpp"

using namespace npcp;

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
} // namespae

std::string reply::rpl_pong(const std::string &server)
{
    return gen_reply({ _m_hostname,
        "PONG",
        server });
}

std::string reply::rpl_welcome(const std::string &source)
{
    return gen_reply({
        _m_hostname,
        "001",
        ":Welcome to the Internet Relay Network\n" +
        source
    });
}

std::string reply::rpl_yourhost(const std::string &ver)
{
    return gen_reply({
        _m_hostname,
        "002",
        ":Your host is " + _hostname + ", running version " + ver
    });
}

std::string reply::rpl_created()
{
    auto t = std::time(nullptr);
    return gen_reply({
        _m_hostname,
        "003",
        ":This server was created " + std::string(std::ctime(&t))
    });
}

std::string reply::rpl_myinfo(const std::string &version,
    const std::string &avaliable_user_modes,
    const std::string &avaliable_channel_modes)
{
    return gen_reply({
        _m_hostname,
        "004",
        _m_hostname,
        version,
        avaliable_user_modes + "\n" + avaliable_channel_modes
    });
}

std::string reply::err_nosuchnick(const std::string &nickname)
{
    return gen_reply({
        _m_hostname,
        "401",
        nickname,
        ":No such nick/channel"
    });
}

std::string reply::err_norecipient(const std::string &command)
{
    return gen_reply({
        _m_hostname,
        "411",
        ":No recipient given",
        "(" + command + ")"
    });
}

std::string reply::err_notexttosend()
{
    return gen_reply({
        _m_hostname,
        "412",
        ":No text to send"
    });
}

std::string reply::err_unknowncommand(const std::string &command)
{
    return gen_reply({
        _m_hostname,
        "421",
        command,
        ":Unknown command"
    });
}

std::string reply::err_nomotd()
{
    return gen_reply({
        _m_hostname,
        "422",
        ":MOTD File is missing"
    });
}

std::string reply::err_nonicknamegiven()
{
    return gen_reply({
        _m_hostname,
        "431",
        ":No nickname given"
    });
}

std::string reply::err_nicknameinuse(const std::string &nick)
{
    return gen_reply({
        _m_hostname,
        "432",
        nick,
        ":Nickname is already in use"
    });
}

std::string reply::err_notregistered()
{
    return gen_reply({
        _m_hostname,
        "451",
        ":You have not registered"
    });
}

std::string reply::err_needmoreparams(const std::string &command)
{
    return gen_reply({
        _m_hostname,
        "461",
        command,
        ":Not enough parameters"
    });
}

std::string reply::err_alreadyregistered()
{
    return gen_reply({
        _m_hostname,
        "462",
        ":Unauthorized command (already registered)"
    });
}
