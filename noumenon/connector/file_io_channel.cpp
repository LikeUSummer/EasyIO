#include "file_io_channel.h"

namespace EasyIO {
FileIOChannel::~FileIOChannel()
{
    if (handle_ >= 0) {
        close(handle_);
    }
}

int64_t FileIOChannel::ReadOnce(uint8_t* data, size_t size)
{
    if (size == 0) {
        return 0;
    }
    int r = read(handle_, data, size);
    if (r > 0) { // 正常读取
        return r;
    }
    if (r < 0 && (errno == EAGAIN ||
                  errno == EWOULDBLOCK ||
                  errno == EINTR)) { // 正常等待
        return 0;
    }
    return -1; // 产生异常
}

int64_t FileIOChannel::WriteOnce(uint8_t* data, size_t size)
{
    if (size == 0) {
        return 0;
    }
    int r = write(handle_, data, size);
    if (r > 0) { // 正常写入
        return r;
    }
    if (r < 0 && (errno == EAGAIN ||
                  errno == EWOULDBLOCK ||
                  errno == EINTR)) { // 正常等待
        return 0;
    }
    return -1; // 产生异常
}

int64_t FileIOChannel::ReadRetry(uint8_t* data, size_t size)
{
    if (size == 0) {
        return 0;
    }
    int64_t n = 0;
#ifdef NON_BLOCK_CHANNEL
    int retryTimes = 0;
    while (retryTimes < MAX_RETRY_TIMES)
#endif
    {
        int r = read(handle_, data + n, size);
        if (r > 0) {
            n += r;
            size -= r;
        } else if (!(r < 0 &&
                    (errno == EAGAIN ||
                     errno == EWOULDBLOCK ||
                     errno == EINTR))) {
            return -1;
        }
        if (size == 0) {
            return n;
        }
#ifdef NON_BLOCK_CHANNEL
        usleep(WAIT_TIME_US);
        ++retryTimes;
#endif
    }
    return n;
}

int64_t FileIOChannel::WriteRetry(uint8_t* data, size_t size)
{
    if (size == 0) {
        return 0;
    }
    int64_t n = 0; // 实际写入字节数
#ifdef NON_BLOCK_CHANNEL
    int retryTimes = 0;
    while (retryTimes < MAX_RETRY_TIMES)
#endif
    {
        int r = write(handle_, data + n, size);
        if (r > 0) {
            n += r;
            size -= r;
        } else if (!(r < 0 &&
                    (errno == EAGAIN ||
                     errno == EWOULDBLOCK ||
                     errno == EINTR))) {
            return -1;
        }
        if (size == 0) {
            return n;
        }
#ifdef NON_BLOCK_CHANNEL
        usleep(WAIT_TIME_US);
        ++retryTimes;
#endif
    }
    return n;
}

bool FileIOChannel::SetNonBlock(bool flag)
{
    int opts;
    opts = fcntl(handle_, F_GETFL);
    if (opts < 0) {
        return false;
    }
    if (flag) {
        opts = opts | O_NONBLOCK;
    } else {
        opts = opts & ~O_NONBLOCK;
    }
    if (fcntl(handle_, F_SETFL, opts) < 0) {
        return false;
    }
    return true;
}

/*
int64_t FileIOChannel::WriteOnce(uint8_t* data, size_t size)
{
    int64_t n = 0;
#if 1
    // 方案一、只 Push 一次，不保证数据全部入队，由调用线程判断和重试
    n = queue_.Push(data, size);
    monitor_->SetRW(handle_);
#else
    // 方案二、尽可能将数据都加入发送队列，但这样不符合异步语义
    int retryTimes = 0;
    while (retryTimes < MAX_RETRY_TIMES) {
        uint32_t r = queue_.Push(data + n, size);
        monitor_->SetRW(handle_);
        size -= r;
        n += r;
        if (size == 0) {
            break;
        }
        usleep(WAIT_TIME_US);
        ++retryTimes;
    }
#endif
    return n;
}

bool FileIOChannel::OnWrite()
{
    if (queue_.Empty()) {
        return true;
    }
    uint8_t buffer[BUFFER_SIZE];
    uint64_t size = queue_.Front(buffer, BUFFER_SIZE);
    int n = write(handle_, buffer, size);
    if (n > 0) {
        queue_.Pop(n);
    }
    return false;
}
*/
}
