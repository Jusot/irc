#ifndef NPCP_PROCESS_HPP
#define NPCP_PROCESS_HPP

#include <mutex>
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

class Process
{
private:
    static bool check_registered(const TcpConnectionPtr& conn);

    static void Process::nick_process(const TcpConnectionPtr& conn, const Message& msg);
    static void Process::user_process(const TcpConnectionPtr& conn, const Message& msg);
    static void Process::quit_process(const TcpConnectionPtr& conn, const Message& msg);
    static void Process::ping_process(const TcpConnectionPtr& conn, const Message& msg);

public:
    static void on_message(const icarus::TcpConnectionPtr& conn, icarus::Buffer* buf /*, Timestamp*/);
};

}


#endif // NPCP_PROCESS_HPP