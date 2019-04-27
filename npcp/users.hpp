#ifndef NPCP_USERS_HPP
#define NPCP_USERS_HPP

#include <vector>
#include <string>
#include <unordered_map>

class ConnectionPtr;
extern std::unordered_map<std::string, ConnectionPtr> users;
extern std::unordered_map<std::string, std::vector<std::string>> channels;

#endif // NPCP_USERS_HPP