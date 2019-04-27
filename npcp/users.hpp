#ifndef NPCP_USERS_HPP
#define NPCP_USERS_HPP

#include <string>
#include <unordered_map>

class ConnectionPtr;
extern std::unordered_map<std::string, ConnectionPtr> users;

#endif // NPCP_USERS_HPP