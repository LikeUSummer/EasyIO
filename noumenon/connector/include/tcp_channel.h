#ifndef TCP_CHANNEL_H
#define TCP_CHANNEL_H

#include "file_io_channel.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>

namespace EasyIO {
// TCP 网络通道
class TCPChannel : public FileIOChannel {
protected:
    std::string ip_;
    int port_ {-1};

public:
    TCPChannel(int handle);
    TCPChannel(const std::string& ip, int port)
        : ip_(ip), port_(port) {}
    virtual ~TCPChannel() {}

public:
    virtual bool Initialise();
    std::string GetIP() { return ip_; }
    int GetPort() { return port_; }
};
}
#endif