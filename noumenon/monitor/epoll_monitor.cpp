#include "epoll_monitor.h"

namespace EasyIO {
EpollMonitor::EpollMonitor(int capacity)
    : Monitor(capacity)
{
    if (capacity > 0) {
        epollEvents_ = (epoll_event*)malloc(capacity * sizeof(epoll_event));
        if (epollEvents_) {
            epollHandle_ = epoll_create(capacity);
        }
    }
}

EpollMonitor::~EpollMonitor()
{
    Stop();
    close(epollHandle_);
    if (epollEvents_) {
        free(epollEvents_);
    }
}

bool EpollMonitor::AddChannel(const std::shared_ptr<Channel>& channel, uint32_t events)
{
    EpollSet(channel->GetHandle(), events);
    if (!Monitor::AddChannel(channel)) {
        return false;
    }
    return true;
}

bool EpollMonitor::AddChannel(const std::shared_ptr<Channel>& channel)
{
    return AddChannel(channel, EPOLLIN);
}

void EpollMonitor::RemoveChannel(int handle)
{
    EpollSet(handle, 0);
    Monitor::RemoveChannel(handle);
}

void EpollMonitor::EpollSet(int handle, uint32_t events)
{
    int r = 0;
    if (events == 0) {
        r = epoll_ctl(epollHandle_, EPOLL_CTL_DEL, handle, NULL);
    } else {
        epoll_event event;
        event.data.fd = handle;
        event.events = events;
        if (channels_.count(handle)) {
            r = epoll_ctl(epollHandle_, EPOLL_CTL_MOD, handle, &event);
        } else {
            r = epoll_ctl(epollHandle_, EPOLL_CTL_ADD, handle, &event);
        }
    }
    if (r < 0) {
        std::cout << "epoll_ctl error: " << errno << std::endl;
    }
}

void EpollMonitor::MainLoop()
{
    while (running_) {
        int n = epoll_wait(epollHandle_, epollEvents_, capacity_, 3000);
        for (int i = 0; i < n; ++i) {
            if (epollEvents_[i].events & EPOLLIN) {
                channels_[epollEvents_[i].data.fd]->OnRead();
            }
        }
    }
}
}