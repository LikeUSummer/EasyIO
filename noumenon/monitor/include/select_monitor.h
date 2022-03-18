#ifndef SELECT_MONITOR_H
#define SELECT_MONITOR_H

#include "monitor.h"

namespace EasyIO {
class SelectMonitor : public Monitor {
protected:
    fd_set readFDs_;
    int maxFD_ {0};

public:
    bool AddChannel(const std::shared_ptr<Channel>& channel) override;

public:
    SelectMonitor(int capacity)
        : Monitor(capacity) {}
    virtual ~SelectMonitor() { Stop(); }

protected:
    void MainLoop() override;
};
}
#endif