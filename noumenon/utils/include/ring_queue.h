#ifndef RING_QUEUE_H
#define RING_QUEUE_H

#include <iostream>
#include <mutex>
#include <cstring>

// 读写完整性控制
// #define RING_QUEUE_INTEGRITY
#define RING_QUEUE_COPY(buffer, size) { \
    uint32_t len = std::min(size, capacity_ - (out_ & (capacity_ - 1))); \
    memcpy(buffer, buffer_ + (out_ & (capacity_ - 1)), len); \
    memcpy(buffer + len, buffer_, size - len); \
}

class RingQueue {
private:
    uint8_t* buffer_ {nullptr};
    uint32_t capacity_ {1024};
    uint32_t size_ {0};
    uint32_t in_ {0};
    uint32_t out_ {0};
    std::mutex forResizeOutOfPop_;
    // std::mutex forResizeOutOfPush_;
    constexpr static uint32_t MAX_CAPACITY = 1 << 11;

public:
    RingQueue();
    ~RingQueue();
    // 无锁接口（用于单生产者与单消费者场景）
    uint32_t Front(uint8_t* buffer, uint32_t size);
    uint32_t Pop(uint32_t size);
    uint32_t Push(uint8_t* buffer, uint32_t size);

    bool Empty() { return size_ == 0; }
    uint32_t GetSize() { return size_; }
    uint32_t GetCapacity() { return capacity_; }
    uint32_t GetResidual() { return capacity_ - size_; }

private:
    bool Resize(uint32_t size);
};
#endif