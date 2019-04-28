#ifndef NPCP_USERS_HPP
#define NPCP_USERS_HPP

#include <string>
#include <unordered_map>

#include "../icarus/icarus/callbacks.hpp"

namespace npcp
{
extern std::unordered_map<std::string, icarus::TcpConnectionPtr> users;

void on_message(const icarus::TcpConnectionPtr &conn, icarus::Buffer *buf /*, Timestamp*/);
}

#endif // NPCP_USERS_HPP