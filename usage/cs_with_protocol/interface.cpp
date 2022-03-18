#include "interface.h"
#include "protocol.h"

#include <fcntl.h>
#include <unistd.h>

namespace EasyIO {
constexpr int BUFFER_SIZE = 1024;

bool PushFile(std::shared_ptr<Channel> channel, const std::string& filePath,
    const std::string& type)
{
    int handle = open(filePath.c_str(), O_RDONLY);
    if (handle == -1) {
        return false;
    }
    Protocol protocol(channel, nullptr);
    protocol.AddFile(filePath);
    protocol.headerMap_["Type"] = type;
    protocol.Write(); // send header
    // send file content
    uint8_t buffer[BUFFER_SIZE];
    ssize_t r {0};
    while (r = read(handle, buffer, BUFFER_SIZE)) {
        if (r < 0) {
            return false;
        }
        channel->WriteOnce(buffer, r);
    }
    close(handle);
    std::cout << "[push file] " << filePath << std::endl;
    return true;
}

bool PushEvent(std::shared_ptr<Channel> channel, const std::string& event)
{
    Protocol protocol(channel, nullptr);
    protocol.AddEvent(event);
    protocol.Write();
    std::cout << "[push event] " << event << std::endl;
    return true;
}

void PullFile(std::shared_ptr<Channel> channel, const std::string& type)
{
    Protocol protocol(channel, nullptr);
    protocol.headerMap_["Code"] = std::to_string(Protocol::Command::PULL_FILE);
    protocol.headerMap_["Type"] = type;
    protocol.Write();
}

void IntroduceMyself(std::shared_ptr<Channel> channel)
{
    Protocol protocol(channel, nullptr);
    protocol.headerMap_["Code"] = std::to_string(Protocol::Command::HELLO);
    protocol.headerMap_["Name"] = channel->GetName();
    protocol.Write();
}
}
