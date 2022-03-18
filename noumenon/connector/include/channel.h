#ifndef CHANNEL_H
#define CHANNEL_H

#include <iostream>
#include <functional>
#include <memory>

#define NON_BLOCK_CHANNEL

namespace EasyIO {
class Channel;

class Channel : public std::enable_shared_from_this<Channel> {
protected:
    constexpr static uint32_t WAIT_TIME_US = 500 * 1000; // 重试等待间隔（500 微秒）
    constexpr static uint32_t TIME_OUT_US = 5 * 1000 * 1000; // 超时时长（5 秒）
    constexpr static uint32_t MAX_RETRY_TIMES = TIME_OUT_US / WAIT_TIME_US; // 最大重试次数

protected:
    int handle_ {-1};
    std::string name_;
    std::function<void (std::shared_ptr<Channel>)> callback_;

public:
    Channel() {}
    Channel(int handle)
        : handle_(handle) {}
    virtual ~Channel() {}

    void SetHandle(int handle) { handle_ = handle; }
    int GetHandle() { return handle_; }
    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetName() { return name_; }
    void SetCallback(const std::function<void (std::shared_ptr<Channel>)>& callback)
        { callback_ = callback; }

public:
    virtual int64_t WriteOnce(uint8_t* data, size_t size) = 0;
    virtual int64_t ReadOnce(uint8_t* data, size_t size) = 0;
    virtual int64_t WriteRetry(uint8_t* data, size_t size) = 0;
    virtual int64_t ReadRetry(uint8_t* data, size_t size) = 0;

    virtual bool OnRead();
    virtual bool OnWrite() { return true; }
};
}
#endif