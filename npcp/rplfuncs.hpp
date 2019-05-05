#ifndef NPCP_RPLFUNCS_HPP
#define NPCP_RPLFUNCS_HPP

#include <string>
#include <vector>

namespace npcp
{
namespace reply
{
std::string rpl_pong(const std::string& server);

std::string rpl_welcome(const std::string& source);
std::string rpl_yourhost(const std::string& ver);
std::string rpl_created();
std::string rpl_myinfo(const std::string& version,
                              const std::string& avaliable_user_modes,
                              const std::string& avaliable_channel_modes);

std::string err_nosuchnick(const std::string &nickname);
std::string err_norecipient(const std::string& command);
std::string err_notexttosend();
std::string err_unknowncommand(const std::string& command);
std::string err_nomotd();
std::string err_nonicknamegiven();
std::string err_nicknameinuse(const std::string& nick);
std::string err_notregistered();
std::string err_needmoreparams(const std::string& command);
std::string err_alreadyregistered();

} // namespace reply
} // namespace npcp

#endif // NPCP_RPLFUNCS_HPP