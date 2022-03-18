#include "ring_queue.h"

RingQueue::RingQueue()
{
    buffer_ = (uint8_t*)malloc(capacity_);
    if (!buffer_) {
        capacity_ = 0;
    }
}

RingQueue::~RingQueue()
{
    if (buffer_) {
        free(buffer_);
    }
}

uint32_t RingQueue::Front(uint8_t* buffer, uint32_t size)
{
    std::lock_guard<std::mutex> lock(forResizeOutOfPop_);
#ifdef RING_QUEUE_INTEGRITY
    if (size > size_) {
        return 0; // 保证读出数据的完整性
    }
#else
    size = std::min(size, size_);
#endif
    RING_QUEUE_COPY(buffer, size);
    return size;
}

uint32_t RingQueue::Pop(uint32_t size)
{
    std::lock_guard<std::mutex> lock(forResizeOutOfPop_);
#ifdef RING_QUEUE_INTEGRITY
    if (size > size_) {
        return 0; // 保证读出数据的完整性
    }
#else
    size = std::min(size, size_);
#endif
    // 无符号整数溢出回绕，左值被置为溢出部分
    // 无符号值上限为 2 的整数次幂，可被 size 整除，因此溢出部分和原值属于同一个剩余类，仅保留溢出部分即可参与等价的计算
    out_ += size;
    size_ = in_ - out_;
    return size;
}

// 虽然 Pop 和 Push 对 size_ 的访问不是线程安全的，但潜在的不一致性只会导致保守结果
uint32_t RingQueue::Push(uint8_t* buffer, uint32_t size)
{
    uint32_t capacityNeed = size + size_;
    if (capacityNeed > capacity_) {
        Resize(capacityNeed);
    }
#ifdef RING_QUEUE_INTEGRITY
    if (size > capacity_ - size_) {
        return 0; // 保证写入数据的完整性
    }
#else
    size = std::min(size, capacity_ - size_);
#endif

    uint32_t len  = std::min(size, capacity_ - (in_ & (capacity_ - 1)));
    memcpy(buffer_ + (in_ & (capacity_ - 1)), buffer, len); // 处理右端
    memcpy(buffer_, buffer + len, size - len); // 处理左端

    in_ += size;
    size_ = in_ - out_;
    return size;
}

bool RingQueue::Resize(uint32_t capacityNeed)
{
    if (capacity_ >= MAX_CAPACITY) {
        return false;
    }
    uint32_t capacity;
    if (capacityNeed < capacity_) {
        capacity = capacityNeed;
    } else {
        capacity = capacity_;
        while (capacity < capacityNeed && capacity < MAX_CAPACITY) {
            capacity = capacity << 1;
        }
    }

    uint8_t* buffer = (uint8_t*)malloc(capacity);
    if (!buffer) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(forResizeOutOfPop_);
        RING_QUEUE_COPY(buffer, size_);
        free(buffer_);
        buffer_ = buffer;
        capacity_ = capacity;
        in_ = size_;
        out_ = 0;
    }
    return true;
}
