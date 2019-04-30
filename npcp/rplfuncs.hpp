#ifndef NPCP_RPLFUNCS_HPP
#define NPCP_RPLFUNCS_HPP

#include <string>

namespace npcp
{
class Reply
{
public:
    static std::string rpl_pong(const std::string& server);

    static std::string rpl_welcome(const std::string& source);
    static std::string rpl_yourhost(const std::string& ver);
    static std::string rpl_created();
    static std::string rpl_myinfo(const std::string& version,
        const std::string& avaliable_user_modes,
        const std::string& avaliable_channel_modes);

    static std::string err_nosuchnick(const std::string &nickname);
    static std::string err_norecipient(const std::string& command);
    static std::string err_notexttosend();
    static std::string err_unknowncommand(const std::string& command);
    static std::string err_nonicknamegiven();
    static std::string err_nicknameinuse(const std::string& nick);
    static std::string err_notregistered();
    static std::string err_needmoreparams(const std::string& command);
    static std::string err_alreadyregistered();

private:
    static std::string gen_reply(const std::vector<std::string>& args);
};
}

#endif // NPCP_RPLFUNCS_HPP