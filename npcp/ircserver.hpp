#ifndef NPCP_IRCSERVER_HPP
#define NPCP_IRCSERVER_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include "../icarus/icarus/tcpserver.hpp"
#include "../icarus/icarus/eventloop.hpp"

namespace npcp
{
class Message;

class IrcServer
{
  public:
    IrcServer(icarus::EventLoop* loop, const icarus::InetAddress& listen_addr, std::string name);

    void start();

  private:
    void on_message(const icarus::TcpConnectionPtr& conn, icarus::Buffer* buf);

    bool check_registered(const icarus::TcpConnectionPtr&);

    void nick_process    (const icarus::TcpConnectionPtr&, const Message&);
    void user_process    (const icarus::TcpConnectionPtr&, const Message&);
    void quit_process    (const icarus::TcpConnectionPtr&, const Message&);
    void privmsg_process (const icarus::TcpConnectionPtr&, const Message&);
    void notice_process  (const icarus::TcpConnectionPtr&, const Message&);
    void ping_process    (const icarus::TcpConnectionPtr&, const Message&);
    void motd_process    (const icarus::TcpConnectionPtr&, const Message&);
    void lusers_process  (const icarus::TcpConnectionPtr&, const Message&);
    void whois_process   (const icarus::TcpConnectionPtr&, const Message&);

    struct Session
    {
        enum class State
        {
            NICK,
            USER,
            REGISTERED
        } state;
        std::string nickname;
        std::string username;
        std::string realname;
    };

    std::mutex nick_conn_mutex_;
    std::unordered_map<std::string, icarus::TcpConnectionPtr> nick_conn_;
    std::unordered_map<icarus::TcpConnectionPtr, Session>     conn_session_;
    std::unordered_map<std::string, std::vector<std::string>> channels_;
    std::unordered_map<std::string, std::string>              user_nick_;

    icarus::TcpServer server_;
};

} // namespace npcp

#endif //NPCP_IRCSERVER_HPP
