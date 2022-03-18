#include "select_monitor.h"

namespace EasyIO {
bool SelectMonitor::AddChannel(const std::shared_ptr<Channel>& channel)
{
    Monitor::AddChannel(channel);
    if (channel->GetHandle() >= maxFD_) {
        maxFD_ = channel->GetHandle();
    }
    return true;
}

void SelectMonitor::MainLoop()
{
    while (running_) {
        FD_ZERO(&readFDs_);
        for (auto& item : channels_) {
            FD_SET(item.first, &readFDs_);
        }
        timeval timeout {3, 0};
        int r = select(maxFD_ + 1, &readFDs_, NULL, NULL, &timeout);
        if (r > 0) {
            for (auto& item : channels_) {
                if (FD_ISSET(item.first, &readFDs_)) {
                    item.second->OnRead();
                }
            }
        }
    }
}
}