#ifndef NPCP_USERS_HPP
#define NPCP_USERS_HPP

#include <string>
#include <unordered_map>

class TcpConnectionPtr;
extern std::unordered_map<std::string, TcpConnectionPtr> users;

/*
void on_message(const TcpConnectionPtr &conn, Buffer *buf)
{
    
}
*/

#endif // NPCP_USERS_HPP