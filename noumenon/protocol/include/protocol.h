#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "channel.h"

#include <algorithm>
#include <map>
#include <memory>

namespace EasyIO {
class Protocol;

class Protocol : public std::enable_shared_from_this<Protocol> {
public:
    enum class Status {
        NETWORK_ERROR = 0,
        PARSE_ERROR,
        EMPTY,
        SIZE_READ,
        HEADER_READ,
        READY
    };

    enum Command {
        PUSH_EVENT = 0,
        PUSH_FILE,
        PULL_FILE,
        HELLO
    };

private:
    static constexpr int BUFFER_SIZE {512};
    static constexpr int CHAR_RANGE {128};
    static uint8_t charFilter_[CHAR_RANGE];

public:
    uint8_t sizeReading_ {0};
    uint32_t headerSize_ {0};
    uint32_t contentSize_ {0};
    std::string header_;
    std::map<std::string, std::string> headerMap_;
    std::string content_;
    Status status_ {Status::EMPTY};

    std::function<void (std::shared_ptr<Protocol>)> callback_;
    // std::function<void (std::shared_ptr<Protocol>)>* handlers_ {nullptr};
    std::shared_ptr<Channel> channel_;

public:
    Protocol() = delete;
    Protocol(std::shared_ptr<Channel> channel,
        std::function<void (std::shared_ptr<Protocol>)> callback);
    virtual ~Protocol() {}
    static void Initialise();

public:
    void AddContent(const std::string& content);
    void AppendContent(const std::string& content);
    void AddFile(const std::string& filePath);
    void AddEvent(const std::string& event);
    const std::string& SerializeHeader();
    std::string Serialize();
    void Read();
    void Write();
    void Clear();

private:
    void ReadSize();
    void ReadHeader();
    void ReadContent();
    bool ParseHeader(const std::string& text);
};
}
#endif