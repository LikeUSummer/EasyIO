#ifndef EPOLL_MONITOR_H
#define EPOLL_MONITOR_H

#include "monitor.h"

#include <sys/epoll.h>

namespace EasyIO {
class EpollMonitor : public Monitor {
protected:
    int epollHandle_ {0};
    epoll_event* epollEvents_ {nullptr};

public:
    EpollMonitor(int capacity);
    virtual ~EpollMonitor();

public:
    bool AddChannel(const std::shared_ptr<Channel>& channel, uint32_t event);
    bool AddChannel(const std::shared_ptr<Channel>& channel) override;
    void RemoveChannel(int handle) override;
    void EpollSet(int handle, uint32_t events);

protected:
    void MainLoop() override;
};
}
#endif