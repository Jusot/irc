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
    void on_connection(const icarus::TcpConnectionPtr& conn);
    void on_message(const icarus::TcpConnectionPtr& conn, icarus::Buffer* buf);

    bool check_registered(const icarus::TcpConnectionPtr&);
    bool check_in_channel(const icarus::TcpConnectionPtr&, const std::string&);

    void nick_process    (const icarus::TcpConnectionPtr&, const Message&);
    void user_process    (const icarus::TcpConnectionPtr&, const Message&);
    void quit_process    (const icarus::TcpConnectionPtr&, const Message&);
    void privmsg_process (const icarus::TcpConnectionPtr&, const Message&);
    void notice_process  (const icarus::TcpConnectionPtr&, const Message&);
    void ping_process    (const icarus::TcpConnectionPtr&, const Message&);
    void motd_process    (const icarus::TcpConnectionPtr&, const Message&);
    void lusers_process  (const icarus::TcpConnectionPtr&, const Message&);
    void whois_process   (const icarus::TcpConnectionPtr&, const Message&);
    void oper_process    (const icarus::TcpConnectionPtr&, const Message&);
    void mode_process    (const icarus::TcpConnectionPtr&, const Message&);
    void join_process    (const icarus::TcpConnectionPtr&, const Message&);
    void part_process    (const icarus::TcpConnectionPtr&, const Message&);
    void topic_process   (const icarus::TcpConnectionPtr&, const Message&);

    struct Session
    {
        enum class State
        {
            NONE,
            NICK,
            USER,
            REGISTERED
        } state;
        std::string nickname;
        std::string username;
        std::string realname;
    };

    struct ChannelInfo
    {
        ChannelInfo() : mode(0) { }
        std::string oper;
        std::vector<std::string> users;
        uint32_t mode;
        std::string topic;
    };

    std::mutex nick_conn_mutex_;
    std::unordered_map<std::string, icarus::TcpConnectionPtr> nick_conn_;
    std::unordered_map<icarus::TcpConnectionPtr, Session>     conn_session_;
    std::unordered_map<std::string, ChannelInfo>              channels_;

    icarus::TcpServer server_;
};

} // namespace npcp

#endif //NPCP_IRCSERVER_HPP
