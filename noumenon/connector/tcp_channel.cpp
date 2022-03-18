#include "tcp_channel.h"

namespace EasyIO {
TCPChannel::TCPChannel(int handle)
    : FileIOChannel(handle)
{
    sockaddr_in addr;
    socklen_t size = sizeof(addr);
    getpeername(handle_, (sockaddr*)&addr, &size);
    port_ = ntohs(addr.sin_port);
    ip_ = inet_ntoa(addr.sin_addr);

#ifdef NON_BLOCK_CHANNEL
    SetNonBlock();
#else
    timeval timeout {0, TIME_OUT_US};
    setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(struct timeval));
    setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
#endif
}

bool TCPChannel::Initialise()
{
    if (handle_ != -1 || ip_.empty() || port_ < 0) {
        return false;
    }

    handle_ = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    sockaddr_in addr {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_aton(ip_.c_str(), &addr.sin_addr);

#ifdef NON_BLOCK_CHANNEL
    SetNonBlock();
    if (connect(handle_, (sockaddr*)(&addr), sizeof(sockaddr)) == -1) {
        if (errno != EINPROGRESS) {
            close(handle_);
            return false;
        }

        int epollHandle = epoll_create(1);
        epoll_event event;
        event.data.fd = handle_;
        event.events = EPOLLIN | EPOLLOUT | EPOLLET; 
        epoll_ctl(epollHandle, EPOLL_CTL_ADD, handle_, &event);

        // 当且仅当epoll_wait调用成功，且有可写但不可读事件，表示连接成功
        bool failed = (epoll_wait(epollHandle, &event, 1, 10000) <= 0) ||
                    (event.events & EPOLLIN) ||
                    !(event.events & EPOLLOUT);
        epoll_ctl(epollHandle, EPOLL_CTL_DEL, handle_, NULL);
        close(epollHandle);

        if (failed) {
            close(handle_);
            return false;
        }
    }
#else
    if (connect(handle_, (sockaddr*)(&addr), sizeof(sockaddr)) == -1) {
        close(handle_);
        return false;
    }
    timeval timeout {0, TIME_OUT_US};
    setsockopt(handle_, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(struct timeval));
    setsockopt(handle_, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
#endif
    return true;
}
}
