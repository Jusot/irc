#include <unistd.h>
#include "ircserver.hpp"
#include "../icarus/icarus/eventloop.hpp"
// #define _DEBUG

int main(int argc, char *argv[])
{
#ifndef _DEBUG
//    daemon(0, 0);
#endif
    using namespace std;
    using namespace npcp;
    using namespace icarus;

    EventLoop loop;
    InetAddress addr(6666);
    IrcServer server(&loop, addr, "irc server");
    server.start();
    loop.loop();
    return 0;
}