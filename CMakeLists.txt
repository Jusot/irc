cmake_minimum_required(VERSION 3.14)
project(npcp)
find_package (Threads)
link_libraries (stdc++fs)
set(CMAKE_CXX_STANDARD 17)

include_directories(npcp)

add_executable(npcp
        npcp/main.cpp
        npcp/message.cpp
        npcp/message.hpp
        npcp/rplfuncs.cpp
        npcp/rplfuncs.hpp
        icarus/icarus/buffer.cpp
        icarus/icarus/buffer.hpp
        icarus/icarus/callbacks.hpp
        icarus/icarus/channel.cpp
        icarus/icarus/channel.hpp
        icarus/icarus/eventloop.cpp
        icarus/icarus/eventloop.hpp
        icarus/icarus/eventloopthread.cpp
        icarus/icarus/eventloopthread.hpp
        icarus/icarus/eventloopthreadpool.cpp
        icarus/icarus/eventloopthreadpool.hpp
        icarus/icarus/inetaddress.cpp
        icarus/icarus/inetaddress.hpp
        icarus/icarus/noncopyable.hpp
        icarus/icarus/poller.cpp
        icarus/icarus/poller.hpp
        icarus/icarus/socket.cpp
        icarus/icarus/socket.hpp
        icarus/icarus/socketsfunc.cpp
        icarus/icarus/socketsfunc.hpp
        icarus/icarus/tcpconnection.cpp
        icarus/icarus/tcpconnection.hpp
        icarus/icarus/timerqueue.cpp
        icarus/icarus/timerqueue.hpp
        icarus/icarus/acceptor.cpp
        icarus/icarus/acceptor.hpp
        icarus/icarus/tcpserver.cpp
        icarus/icarus/tcpserver.hpp
        npcp/ircserver.cpp
        npcp/ircserver.hpp)

target_link_libraries (npcp ${CMAKE_THREAD_LIBS_INIT})
