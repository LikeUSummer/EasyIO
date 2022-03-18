#ifndef MONITOR_H
#define MONITOR_H

#include "channel.h"

#include <map>
#include <memory>
#include <thread>

#include <unistd.h>

namespace EasyIO {
class Monitor {
protected:
    std::map<int, std::shared_ptr<Channel>> channels_;
    int capacity_ {0};

    std::unique_ptr<std::thread> thread_;
    bool running_ {false};

public:
    Monitor(int capacity);
    virtual ~Monitor();
    virtual bool AddChannel(const std::shared_ptr<Channel>& channel);
    virtual void RemoveChannel(int handle);
    virtual bool Start();
    virtual void Stop();
    void Write(uint8_t* data, size_t size);

protected:
    virtual void MainLoop() = 0;
};
}
#endif
