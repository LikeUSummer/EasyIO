#ifndef TCP_LISTEN_CHANNEL_H
#define TCP_LISTEN_CHANNEL_H

#include "tcp_channel.h"

namespace EasyIO {
class TCPListenChannel : public TCPChannel {
public:
    TCPListenChannel(const std::string& ip, int port)
        : TCPChannel(ip, port) {}
    virtual ~TCPListenChannel() {}

public:
    bool Initialise() override;

protected:
    bool OnRead() override; // read 动作被实现为 accept
    int OnConnect();
};
}
#endif