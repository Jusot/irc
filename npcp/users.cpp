#include <string>
#include <unordered_map>

#include "users.hpp"

std::unordered_map<std::string, ConnectionPtr> users;
std::unordered_map<std::string, std::vector<std::string>> channels;