#include "monitor.h"

namespace EasyIO {
Monitor::Monitor(int capacity)
{
    if (capacity > 0) {
        capacity_ = capacity;
    }
}

Monitor::~Monitor()
{
    channels_.clear();
}

bool Monitor::AddChannel(const std::shared_ptr<Channel>& channel)
{
    if (channels_.size() >= capacity_) {
        return false;
    }
    channels_[channel->GetHandle()] = channel;
    return true;
}

void Monitor::RemoveChannel(int handle)
{
    channels_.erase(handle);
}

bool Monitor::Start()
{
    if (running_) {
        return true;
    }
    running_ = true;
    thread_ = std::make_unique<std::thread>(&Monitor::MainLoop, this);
    return true;
}

void Monitor::Stop()
{
    if (running_) {
        running_ = false;
        if (thread_->joinable()) {
            thread_->join();
        }
    }
}

void Monitor::Write(uint8_t* data, size_t size)
{
    for (auto item : channels_) {
        int n = item.second->WriteOnce(data, size);
    }
}
}