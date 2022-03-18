#ifndef FILE_IO_CHANNEL_H
#define FILE_IO_CHANNEL_H

#include "channel.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace EasyIO {
class FileIOChannel : public Channel {
public:
    FileIOChannel() {}
    FileIOChannel(int handle)
        : Channel(handle) {}
    virtual ~FileIOChannel();

public:
    int64_t ReadOnce(uint8_t* data, size_t size) override;
    int64_t WriteOnce(uint8_t* data, size_t size) override;
    int64_t ReadRetry(uint8_t* data, size_t size) override;
    int64_t WriteRetry(uint8_t* data, size_t size) override;

public:
    bool SetNonBlock(bool flag = true);
};
}
#endif