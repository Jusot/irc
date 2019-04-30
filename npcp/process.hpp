#ifndef NPCP_PROCESS_HPP
#define NPCP_PROCESS_HPP

#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>

#include "../icarus/icarus/callbacks.hpp"

namespace npcp
{
class Message;

struct Session
{
    std::string nickname;
    std::string realname;
};

class Process
{
private:
    static bool check_registered(const icarus::TcpConnectionPtr&);

    static void nick_process(const icarus::TcpConnectionPtr&, const Message&);
    static void user_process(const icarus::TcpConnectionPtr&, const Message&);
    static void quit_process(const icarus::TcpConnectionPtr&, const Message&);
    static void privmsg_process(const icarus::TcpConnectionPtr&, const Message&);
    static void notice_process(const icarus::TcpConnectionPtr&, const Message&);
    static void ping_process(const icarus::TcpConnectionPtr&, const Message&);

public:
    static void on_message(const icarus::TcpConnectionPtr& conn, icarus::Buffer* buf /*, Timestamp*/);
};

}


#endif // NPCP_PROCESS_HPP