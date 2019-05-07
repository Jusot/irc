#ifndef NPCP_RPLFUNCS_HPP
#define NPCP_RPLFUNCS_HPP

#include <string>

namespace npcp
{
namespace reply
{
std::string rpl_pong(const std::string& server);

std::string rpl_welcome(const std::string &nick,
    const std::string &user, 
    const std::string &host);                               // 001
std::string rpl_yourhost(const std::string& nick,
    const std::string &ver);                                // 002
std::string rpl_created(const std::string& nick);                                  // 003
std::string rpl_myinfo(const std::string& nick,
    const std::string& version,
    const std::string& avaliable_user_modes,
    const std::string& avaliable_channel_modes);            // 004
std::string rpl_luserclient(const std::string& nick,
    int, int, int);                                         // 251
std::string rpl_luserop(const std::string& nick, int);      // 252
std::string rpl_luserunknown(const std::string& nick, int); // 253
std::string rpl_luserchannels(const std::string& nick, int);// 254
std::string rpl_luserme(const std::string& nick, int, int); // 255
std::string rpl_whoisuser(const std::string& nick,          // 311
                          const std::string& user,
                          const std::string& realname);
std::string rpl_whoisserver(const std::string& nick);       // 312
std::string rpl_endofwhois(const std::string& nick);        // 318
std::string rpl_youareoper(const std::string& nick);        // 381

std::string err_nosuchnick(const std::string& nickname);    // 401
std::string err_norecipient(const std::string& command);    // 411
std::string err_notexttosend();                             // 412
std::string err_unknowncommand(const std::string& command); // 421
std::string err_nomotd(const std::string &nick);            // 422
std::string err_nonicknamegiven();                          // 431
std::string err_nicknameinuse(const std::string& nick);     // 433
std::string err_notregistered(const std::string& nick);     // 451
std::string err_needmoreparams(const std::string& nick,
    const std::string& command);                            // 461
std::string err_alreadyregistered();                        // 462
std::string err_passwdmismatch(const std::string& nick);    // 464
std::string err_umodeunknownflag(const std::string& nick);  // 501
std::string err_usersdontmatch(const std::string& nick);    // 502
} // namespace reply
} // namespace npcp

#endif // NPCP_RPLFUNCS_HPP