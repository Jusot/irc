#include <unistd.h>
#include "ircserver.hpp"
#include "../icarus/icarus/eventloop.hpp"

#define _DEBUG

int main(int argc, char *argv[])
{
#ifndef _DEBUG
//    daemon(0, 0);
#endif

    icarus::EventLoop loop;
    icarus::InetAddress addr(7776);

    npcp::IrcServer server(&loop, addr, "irc server");
    
    server.start();
    loop.loop();

    return 0;
}