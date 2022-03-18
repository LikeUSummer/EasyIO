#include "interface.h"

#include "tcp_channel.h"
#include "tcp_listen_channel.h"
#include "ssl_channel.h"
#include "ssl_listen_channel.h"
#include "epoll_monitor.h"
#include "protocol.h"
#include "thread_pool.h"
#include "utils.h"

#include <fcntl.h>
#include <unistd.h>

#include <map>

using namespace EasyIO;

const std::string IP = "127.0.0.1";
constexpr int PORT = 9999;
constexpr int BUFFER_SIZE = 1024;
constexpr int MONITOR_CAPACITY = 32;
constexpr int THREAD_POOL_SIZE = 3;
const std::string LOG_ROOT_DIR = "test/server/";
std::map<std::string, std::shared_ptr<Channel>> g_agentMap;

void OnPushFile(std::shared_ptr<Protocol> protocol)
{
    std::string dirPath = LOG_ROOT_DIR + protocol->channel_->GetName() + "/" +
        protocol->headerMap_["Type"] + "/";
    if (!CreateDir(dirPath)) {
        return;
    }
    std::string filePath = dirPath + protocol->headerMap_["Name"];
    int handle = open(filePath.c_str(), O_RDWR | O_CREAT, 0777);
    if (handle == -1) {
        return;
    }
    uint8_t data[BUFFER_SIZE];
    uint32_t rest = std::atol(protocol->headerMap_["Size"].c_str());
    int64_t size = 0;
    while (rest) {
        size = rest < BUFFER_SIZE ? rest : BUFFER_SIZE;
        size = protocol->channel_->ReadOnce(data, size);
        if (size < 0) {
            break;
        }
        write(handle, data, size);
        rest -= size;
    }
    close(handle);
    if (size < 0) {
        remove(filePath.c_str());
    } else {
        std::cout << "[receive file] " << filePath << std::endl;
    }
}

void OnPushEvent(std::shared_ptr<Protocol> protocol)
{
    std::cout << "[receive event] " <<  protocol->content_ << std::endl;
}

void OnPullFile(std::shared_ptr<Protocol> protocol)
{
    return;
}

void OnHello(std::shared_ptr<Protocol> protocol)
{
    protocol->channel_->SetName(protocol->headerMap_["Name"]);
    g_agentMap[protocol->headerMap_["Name"]] = protocol->channel_;
}

std::function<void (std::shared_ptr<Protocol>)> g_handlers[] {
    OnPushEvent,
    OnPushFile,
    OnPullFile,
    OnHello
};

void OnRequest(std::shared_ptr<Protocol> protocol)
{
    int command = std::atoi(protocol->headerMap_["Code"].c_str());
    if (command >= Protocol::Command::PUSH_EVENT &&
        command <= Protocol::Command::HELLO) {
        g_handlers[command](protocol);
    }
}

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    CreateDir(LOG_ROOT_DIR);
#ifdef USESSL
    SSLChannel::OpenSSL("key/server/device.crt", "key/server/device.key");
    auto channel = std::make_unique<SSLListenChannel>(IP, PORT);
#else
    auto channel = std::make_unique<TCPListenChannel>(IP, PORT);
#endif
    EpollMonitor monitor(MONITOR_CAPACITY);
    ThreadPool threadPool(THREAD_POOL_SIZE);
    if (!channel->Initialise()) {
        return 1;
    }
    channel->SetCallback([&monitor, &threadPool](std::shared_ptr<Channel> client) {
        auto protocol = std::make_shared<Protocol>(client, OnRequest);
        client->SetCallback([&monitor, &threadPool, protocol](std::shared_ptr<Channel> self) {
            threadPool.Push([&monitor, protocol, self]() {
                protocol->Read();
                monitor.EpollSet(self->GetHandle(), EPOLLIN | EPOLLET | EPOLLONESHOT);
            });
        });
        monitor.AddChannel(client, EPOLLIN | EPOLLET | EPOLLONESHOT);
    });
    monitor.AddChannel(std::move(channel));
    monitor.Start();

    while (true) {
        std::string code;
        std::cin >> code;
        if (code == "pullfile") {
            if (g_agentMap["client001"].use_count()) {
                PullFile(g_agentMap["client001"], "error");
            }
        }
    }
    return 0;
}