#ifndef SSL_LISTEN_CHANNEL_H
#define SSL_LISTEN_CHANNEL_H

#include "tcp_listen_channel.h"

namespace EasyIO {
class SSLListenChannel : public TCPListenChannel {
public:
    SSLListenChannel(const std::string& ip, int port)
        : TCPListenChannel(ip, port) {}
    virtual ~SSLListenChannel() {}

protected:
    bool OnRead() override;
};
}
#endif