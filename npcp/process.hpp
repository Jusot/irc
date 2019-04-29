#ifndef NPCP_PROCESS_HPP
#define NPCP_PROCESS_HPP

#include <vector>
#include <string>
#include <unordered_map>

#include "../icarus/icarus/callbacks.hpp"

namespace npcp
{
struct Session
{
    std::string nickname;
    std::string realname;
};

extern std::unordered_map<std::string, icarus::TcpConnectionPtr> nick_conn;
extern std::unordered_map<icarus::TcpConnectionPtr, Session> conn_session;
extern std::unordered_map<std::string, std::vector<std::string>> channels;

void on_message(const icarus::TcpConnectionPtr& conn, icarus::Buffer* buf /*, Timestamp*/);
}


#endif // NPCP_PROCESS_HPP