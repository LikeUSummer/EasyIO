#include "interface.h"

#include "tcp_channel.h"
#include "tcp_listen_channel.h"
#include "ssl_channel.h"
#include "ssl_listen_channel.h"
#include "epoll_monitor.h"
#include "protocol.h"
#include "utils.h"

#include <fcntl.h>
#include <unistd.h>

using namespace EasyIO;

const std::string IP = "127.0.0.1";
constexpr int PORT = 9999;
constexpr int MONITOR_CAPACITY = 1;
constexpr int FILE_COUNT = 5;
const std::string LOG_ROOT_DIR = "test/client/";
const std::string EVENT_TEST = "{'ID':'4399', 'Name':'Summer', 'Age':18, 'Color':'Orange'}";

bool CreateTestFiles()
{
    std::string subDirs[] {"error", "warn", "info"};
    for (const auto& subDir : subDirs) {
        std::string dirPath = LOG_ROOT_DIR + subDir + "/";
        if (!CreateDir(dirPath)) {
            return false;
        }
        for (int i = 0; i < FILE_COUNT; ++i) {
            std::string filePath = dirPath + subDir + std::to_string(i + 1) + ".log";
            if (access(filePath.c_str(), F_OK) == 0) {
                continue;
            }
            int handle = open(filePath.c_str(), O_RDWR | O_CREAT, 0777);
            if (handle < 0) {
                return false;
            }
            int count = 500;
            std::string pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
            while (count--) {
                write(handle, pattern.c_str(), pattern.size());
            }
            close(handle);
        }
    }
    return true;
}

void OnRequest(std::shared_ptr<Protocol> protocol)
{
    int command = std::atoi(protocol->headerMap_["Code"].c_str());
    if (command == Protocol::Command::PULL_FILE) {
        std::string dirPath = LOG_ROOT_DIR + protocol->headerMap_["Type"] + "/";
        ForEachFile(dirPath, std::bind(PushFile, protocol->channel_, std::placeholders::_1,
            protocol->headerMap_["Type"]));
    }
}

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!CreateTestFiles()) {
        std::cout << "create test files failed.\n";
        return 1;
    }
    EpollMonitor monitor(MONITOR_CAPACITY);
#ifdef USESSL
    SSLChannel::OpenSSL("key/client/device.crt", "key/client/device.key");
    auto channel = std::make_shared<SSLChannel>(IP, PORT);
#else
    auto channel = std::make_shared<TCPChannel>(IP, PORT);
#endif
    if (!channel->Initialise()) {
        return 1;
    }
    channel->SetName("client001");
    auto protocol = std::make_shared<Protocol>(channel, OnRequest);
    channel->SetCallback([&monitor, protocol](std::shared_ptr<Channel> self) {
        protocol->Read();
    });
    monitor.AddChannel(channel);
    monitor.Start();

    IntroduceMyself(channel);

    while (true) {
        std::string code;
        std::cin >> code;
        if (code == "pushevent") {
            PushEvent(channel, EVENT_TEST);
        } else if (code == "pushfile") {
            ForEachFile(LOG_ROOT_DIR + "info/",
                std::bind(PushFile, channel, std::placeholders::_1, "info"));
        }
    }
    return 0;
}