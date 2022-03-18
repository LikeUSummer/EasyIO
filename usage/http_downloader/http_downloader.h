#ifndef HTTP_DOWNLOADER_H
#define HTTP_DOWNLOADER_H

#include "epoll_monitor.h"

#include <map>
#include <mutex>
#include <string>

namespace EasyIO {
class Downloader {
public:
    static constexpr int BUFFER_SIZE {1024};
    static constexpr int MAX_CHANNEL {64};
    static std::string REQUEST_HEADER;
    static EpollMonitor monitor_;

public:
    static void CreateFile(const std::string& filePath, uint64_t fileSize);
    static void ParseHeader(const std::string& headerRaw, std::map<std::string, std::string>& headerMap);

public:
    std::string url_;
    std::string hostName_;
    std::string ip_;
    int port_ {80};

    std::string headerRaw_;
    std::map<std::string, std::string> headerMap_;

    std::string fileName_;
    uint64_t fileSize_ {0};
    uint64_t sizeRead_ {0};
    std::mutex lockForSizeRead_;

    int nThread_ {1};

public:
    Downloader(const std::string& url, const std::string& fileName, int nThread);
    bool VisitForFileInfo();
    void Download();
};
}
#endif