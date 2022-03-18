#include "protocol.h"

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

namespace EasyIO {
uint8_t Protocol::charFilter_[CHAR_RANGE] {0};

Protocol::Protocol(std::shared_ptr<Channel> channel,
    std::function<void (std::shared_ptr<Protocol>)> callback)
    : channel_(channel), callback_(callback)
{
    headerMap_["Size"] = "0";
}

void Protocol::Initialise()
{
    for (char c = 0; c < 26; ++c) { // 26 : count of English letters
        charFilter_['a' + c] = 1;
        charFilter_['A' + c] = 1;
    }
    for (char c = '0'; c <= '9'; ++c) {
        charFilter_[c] = 1;
    }
    charFilter_['-'] = 1;
    charFilter_['_'] = 1;
    charFilter_['.'] = 1;
}

struct Initialiser {
    Initialiser()
    {
        Protocol::Initialise();
    }
};
Initialiser g_autoInitialise;

void Protocol::AddContent(const std::string& content)
{
    content_ = content;
    headerMap_["Size"] = std::to_string(content_.size());
}

void Protocol::AppendContent(const std::string& content)
{
    content_.append(content);
    headerMap_["Size"] = std::to_string(content_.size());
}

void Protocol::AddFile(const std::string& filePath)
{
    int handle = open(filePath.c_str(), O_RDONLY);
    if (handle == -1) {
        return;
    }
    struct stat fileState;
    fstat(handle, &fileState);
    headerMap_["Size"] = std::to_string(fileState.st_size);
    auto iter = filePath.rbegin();
    while (iter != filePath.rend()) {
        if (!charFilter_[*iter]) {
            break;
        }
        ++iter;
    }
    headerMap_["Code"] = std::to_string(Command::PUSH_FILE);
    headerMap_["Name"] = filePath.substr(filePath.rend() - iter);
    close(handle);
}

void Protocol::AddEvent(const std::string& event)
{
    headerMap_["Code"] = std::to_string(Command::PUSH_EVENT);
    AddContent(event);
}

const std::string& Protocol::SerializeHeader()
{
    header_.clear();
    for (auto& item : headerMap_) {
        header_ += item.first + ":" + item.second + "\n";
    }
    return header_;
}

std::string Protocol::Serialize()
{
    return SerializeHeader() + content_;
}

bool Protocol::ParseHeader(const std::string& text)
{
    std::string token, key;
    int count = 0;
    for (char c : text) {
        if (charFilter_[c]) {
            token.push_back(c);
        } else if (token.length()) {
            if (key.empty()) {
                key = token;
            } else {
                headerMap_[key] = token;
                key.clear();
            }
            token.clear();
        }
        if (c == ':') {
            ++count;
        }
    }
    if (key.length() && token.empty()) {
        return false;
    }
    if (token.length()) {
        headerMap_[key] = token;
    }
    if (headerMap_.size() != count) {
        return false;
    }
    return true;
}

void Protocol::ReadSize()
{
    uint8_t size = sizeof(headerSize_) - sizeReading_;
    int64_t r = channel_->ReadOnce((uint8_t*)&headerSize_, size);
    if (r < 0) {
        status_ = Status::NETWORK_ERROR;
        return;
    }
    sizeReading_ += r;
    if (sizeReading_ == sizeof(headerSize_)) {
        status_ = Status::SIZE_READ;
    }
}

void Protocol::ReadHeader()
{
    if (headerSize_) {
        char data[BUFFER_SIZE + 1] {0};
        uint32_t size = headerSize_ - header_.size();
        size = size < BUFFER_SIZE ? size : BUFFER_SIZE;
        int64_t r = channel_->ReadOnce((uint8_t*)data, size);
        if (r < 0) {
            status_ = Status::NETWORK_ERROR;
            return;
        }
        if (r > 0) {
            data[r] = 0;
            header_.append(data);
        }
        if (header_.size() == headerSize_) {
            if (ParseHeader(header_)) {
                contentSize_ = std::atol(headerMap_["Size"].c_str());
                status_ = Status::HEADER_READ;
            } else {
                status_ = Status::PARSE_ERROR;
            }
        } else if (r == BUFFER_SIZE) {
            ReadHeader();
        }
    }
}

void Protocol::ReadContent()
{
    if (contentSize_) {
        char data[BUFFER_SIZE + 1] {0};
        uint32_t size = contentSize_ - content_.size();
        size = size < BUFFER_SIZE ? size : BUFFER_SIZE;
        int64_t r = channel_->ReadOnce((uint8_t*)data, size);
        if (r < 0) {
            status_ = Status::NETWORK_ERROR;
            return;
        }
        if (r > 0) {
            data[r] = 0;
            content_.append(data);
            if (content_.size() == contentSize_) {
                status_ = Status::READY;
            } else if (r == BUFFER_SIZE) {
                ReadContent();
            }
        }
    } else {
        status_ = Status::READY;
    }
}

void Protocol::Read()
{
    if (status_ < Status::EMPTY) {
        return;
    }
    if (status_ == Status::EMPTY) {
        ReadSize();
    }
    if (status_ == Status::SIZE_READ) {
        ReadHeader();
    }
    if (status_ == Status::HEADER_READ) {
        if (std::atoi(headerMap_["Code"].c_str()) != Command::PUSH_FILE) {
            ReadContent();
        } else { // Content of "File" type would be handled separately
            status_ = Status::READY;
        }
    }
    if (status_ == Status::READY) {
        callback_(shared_from_this());
        Clear();
    }
}

void Protocol::Write()
{
    SerializeHeader();
    headerSize_ = header_.size();
    int64_t r = channel_->WriteOnce((uint8_t*)(&headerSize_), sizeof(headerSize_));
    r = channel_->WriteOnce((uint8_t*)(header_.c_str()), header_.size());
    r = channel_->WriteOnce((uint8_t*)(content_.c_str()), content_.size());
}

void Protocol::Clear()
{
    header_.clear();
    content_.clear();
    headerMap_.clear();
    sizeReading_ = 0;
    headerSize_ = 0;
    contentSize_ = 0;
    status_ = Status::EMPTY;
}
}