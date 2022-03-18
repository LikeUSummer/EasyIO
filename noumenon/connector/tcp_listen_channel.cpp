#include "tcp_listen_channel.h"

namespace EasyIO {
bool TCPListenChannel::Initialise()
{
    if (ip_.empty() || port_ == -1 || handle_ != -1) {
        return false;
    }

    handle_ = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    sockaddr_in addr {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_aton(ip_.c_str(), &addr.sin_addr);

    if (bind(handle_, (sockaddr*)(&addr), sizeof(sockaddr_in))) {
        std::cout << "bind failed\n";
        return false;
    }
    if (listen(handle_, 10)) {
        std::cout << "listen failed\n";
        return false;
    }
#ifdef NON_BLOCK_CHANNEL
    SetNonBlock();
#endif
    return true;
}

int TCPListenChannel::OnConnect()
{
    sockaddr_in addr = {0};
    socklen_t size = sizeof(sockaddr_in);
    int handle = accept(handle_, (sockaddr*)&addr, &size);
    return handle;
}

bool TCPListenChannel::OnRead()
{
#ifdef NON_BLOCK_CHANNEL
    while (1)
#endif
    {
        int handle = OnConnect();
        if (handle < 0) {
            return false;
        }
        auto channel = std::make_shared<TCPChannel>(handle);
        callback_(channel); // 通过回调函数传出新建的通道
    }
    return true;
}
}