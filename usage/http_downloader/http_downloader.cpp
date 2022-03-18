#include "http_downloader.h"

#include "tcp_channel.h"
#include "tcp_listen_channel.h"
#include "thread_pool.h"
#include "utils.h"

#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <fstream>
#include <future>
#include <memory>
#include <vector>

namespace EasyIO {
std::string Downloader::REQUEST_HEADER = "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n"
    "user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/98.0.4758.102 Safari/537.36 Edg/98.0.1108.62\r\n"
    "Connection: keep-alive\r\n"
    "Range: bytes=%ld-%ld\r\n"
    "\r\n";
EpollMonitor Downloader::monitor_(Downloader::MAX_CHANNEL);

Downloader::Downloader(const std::string& url, const std::string& fileName, int nThread)
    : url_(url), fileName_(fileName), nThread_(nThread)
{
    char lastChar {0};
    int index = 0;
    while (++index < url.length()) {
        if (url[index] == '/' && lastChar == '/') {
            break;
        }
        lastChar = url[index];
    }
    while (++index < url.length()) {
        if (url[index] == '/') {
            break;
        }
        hostName_.push_back(url[index]);
    }

    if (nThread_ > MAX_CHANNEL) {
        nThread_ = MAX_CHANNEL;
    }
}

// 解析 HTTP 协议头
void Downloader::ParseHeader(const std::string& headerRaw, std::map<std::string, std::string>& headerMap)
{
    std::string key;
    std::string value;
    int index = 0;

    auto lower = [](char c) -> char {
        if (c >= 'A' && c <= 'Z') {
            return c - 'A' + 'a';
        }
        return c;
    };

    headerMap.clear();
    while (index < headerRaw.length() - 2) { // 2: the last "\r\n"
        if (value.empty()) {
            if (headerRaw[index] != ':') {
                key.push_back(lower(headerRaw[index]));
            } else {
                value.push_back(lower(headerRaw[++index]));
            }
        } else {
            if (headerRaw[index] == '\r' && headerRaw[index + 1] == '\n') {
                headerMap.insert({key, value});
#ifdef DEBUG_MODE
                std::cout << key << ':' << value << std::endl;
#endif
                key.clear();
                value.clear();
                ++index;
            } else {
                value.push_back(headerRaw[index]);
            }
        }
        ++index;
    }
}

bool Downloader::VisitForFileInfo()
{
    // 解析 IP 地址
    struct hostent* hostInfo = gethostbyname(hostName_.c_str());
    if (hostInfo == nullptr) {
        std::cout << "DNS error\n";
        return false;
    }
    ip_ = inet_ntoa(*((struct in_addr *)hostInfo->h_addr));

    // 创建通道对象并设置回调函数
    auto channel = std::make_shared<TCPChannel>(ip_, port_);
    std::promise<bool> donePromise;
    channel->SetCallback([this, &donePromise](std::shared_ptr<Channel> self) {
        uint8_t data[BUFFER_SIZE];
        int64_t count = self->ReadOnce(data, BUFFER_SIZE - 1);
        if (count > 0) {
            data[count] = 0;
            headerRaw_.append((char*)data);
            if (headerRaw_.find("\r\n\r\n") >= 0) {
                if (headerRaw_.back() == EOF) {
                    headerRaw_.pop_back();
                }
                Downloader::ParseHeader(headerRaw_, headerMap_);
                Downloader::monitor_.RemoveChannel(self->GetHandle());
                donePromise.set_value(true);
            }
        }
    });

    // 连接服务器并发送请求报文
    if (!channel->Initialise()) {
        std::cout << "connect failed\n";
        return false;
    }
    char data[BUFFER_SIZE] {0};
    sprintf(data, REQUEST_HEADER.c_str(),
        url_.c_str(), hostName_.c_str(), 0L, 0L);
    channel->WriteOnce((uint8_t*)data, strlen(data));

    monitor_.AddChannel(channel);
    monitor_.Start();
    donePromise.get_future().wait(); // 等待异步执行结果

    if (headerMap_.count("content-range")) {
        auto& range = headerMap_["content-range"];
        std::string size = range.substr(range.find_last_of('/') + 1);
        fileSize_ = std::stold(size);
    } else if (headerMap_.count("content-length")) {
        fileSize_ = std::stold(headerMap_["content-length"]);
    }
    std::cout << "[File Size] " << fileSize_ << std::endl;
    return fileSize_ > 0;
}

// 创建文件，并填充空数据
void Downloader::CreateFile(const std::string& filePath, uint64_t fileSize)
{
    std::fstream file(filePath, std::ios::binary | std::ios::out);
    uint8_t patch[BUFFER_SIZE] {0};
    uint64_t bytesNeed = fileSize;
    while (bytesNeed) {
        uint64_t bytesWrite = bytesNeed > BUFFER_SIZE ? BUFFER_SIZE : bytesNeed;
        file.write((const char*)patch, bytesWrite);
        bytesNeed -= bytesWrite;
    }
    file.close();
}

// 下载
void Downloader::Download()
{
    auto timeBegin = std::chrono::steady_clock::now();
    if (!VisitForFileInfo()) {
        return;
    }
    // Downloader::CreateFile(fileName_, fileSize_); // 占位文件并不是必要的

    struct ChannelInfo {
        std::shared_ptr<TCPChannel> channel_;
        uint64_t bytesNeed_ {0}; // 需要下载的区块大小
        uint64_t offset_ {0}; // 文件偏移量
        std::string header_; // HTTP 协议头
        bool jumpHeader_ {false}; // 指示当前是否已跳过协议头
        std::fstream file_; // 文件对象
    };
    uint64_t offset = 0;
    uint64_t batchSize = fileSize_ / nThread_;
    int restSize = fileSize_ % nThread_;
    std::promise<bool> donePromise; // 用于阻塞获取异步下载完成的标志
    std::vector<ChannelInfo> channelInfoList(nThread_);
#ifdef USE_THREAD_POOL
    ThreadPool threadPool(nThread_); // 当前设置线程池容量和通道数一样，但这种一致性不是必需的
#endif
    // 初始化各个通道的参数
    for (auto& channelInfo : channelInfoList) {
        channelInfo.channel_ = std::make_shared<TCPChannel>(ip_, port_);
        channelInfo.bytesNeed_ = batchSize;
        if (restSize) { // 余量均分到各个通道
            ++channelInfo.bytesNeed_;
            --restSize;
        }
        channelInfo.file_.open(fileName_, std::ios::binary | std::ios::out);
        channelInfo.file_.seekp(offset, std::ios::beg); // 每个通道对应不同的文件偏移量
        channelInfo.offset_ = offset;
        offset += channelInfo.bytesNeed_;

        // 这里一定要把 channel 作为参数而非捕获变量，否则会导致智能指针循环引用（内存泄漏）
        auto callback = [this, &channelInfo, &donePromise]
            (std::shared_ptr<Channel> channel) {
            while(true) { // while: 一次触发尽可能读完缓冲区数据
            uint8_t data[BUFFER_SIZE + 1];
            int64_t count = channel->ReadOnce(data, BUFFER_SIZE);
            if (count > 0) {
                int offset = 0;
                if (!channelInfo.jumpHeader_) {
                    data[count] = 0;
                    channelInfo.header_.append((char*)data);
                    int index = channelInfo.header_.find("\r\n\r\n");
                    if (index >= 0) {
                        channelInfo.jumpHeader_ = true;
                        offset = index + 4; // 4: jump over "\r\n\r\n"
                        count -= offset;
                    } else {
                        continue;
                    }
                }
                if (count > channelInfo.bytesNeed_) {
                    std::cout << "ignore " << count - channelInfo.bytesNeed_ << " bytes\n";
                    count = channelInfo.bytesNeed_;
                }

                channelInfo.file_.write((const char*)data + offset, count);

                channelInfo.bytesNeed_ -= count;
                {
                    std::unique_lock<std::mutex> guard(lockForSizeRead_);
                    sizeRead_ += count;
                    // std::cout << int((1.0 * sizeRead_ / fileSize_) * 100) << '%' << std::endl;
                    if (sizeRead_ == fileSize_) {
                        donePromise.set_value(true);
                    }
                }
                if (channelInfo.bytesNeed_== 0) {
                    channelInfo.file_.close();
                    Downloader::monitor_.RemoveChannel(channel->GetHandle());
                    return;
                }
            } else if (count == 0) {
                // 重置 EPOLLONESHOT
                Downloader::monitor_.EpollSet(channel->GetHandle(), EPOLLIN | EPOLLET | EPOLLONESHOT);
                return;
            }
            }
        };

        channelInfo.channel_->SetCallback([
#ifdef USE_THREAD_POOL
             &threadPool,
#endif
            callback] (std::shared_ptr<Channel> self) mutable {
#ifdef USE_THREAD_POOL
            threadPool.Push(callback, self);
#else
            callback(self);
#endif
        });
    }

    for (auto& channelInfo : channelInfoList) {
        // 连接服务器并发送请求
        if (!channelInfo.channel_->Initialise()) {
            std::cout << "connect failed\n";
            return;
        }
        char data[BUFFER_SIZE] {0};
        sprintf(data, REQUEST_HEADER.c_str(),
            url_.c_str(), hostName_.c_str(),
            channelInfo.offset_, channelInfo.offset_ + channelInfo.bytesNeed_ - 1);
        channelInfo.channel_->WriteRetry((uint8_t*)data, strlen(data));
        Downloader::monitor_.AddChannel(channelInfo.channel_, EPOLLIN | EPOLLET | EPOLLONESHOT);
    }
    monitor_.Start();
    donePromise.get_future().wait();
    auto timeEnd = std::chrono::steady_clock::now();
    double timeUse = std::chrono::duration<double, std::milli>(timeEnd - timeBegin).count();
    std::cout << "download successful in " << timeUse << " ms\n";
#ifdef DEBUG_MODE
    // 输出引用计数以检查内存泄漏，此处预期值为 1
    for (auto& channelInfo : channelInfoList) {
        std::cout << "channel ref: " << channelInfo.channel_.use_count() << std::endl;
    }
#endif
}
}
