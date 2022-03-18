#include "tcp_channel.h"
#include "tcp_listen_channel.h"
#ifdef USESSL
#include "ssl_channel.h"
#include "ssl_listen_channel.h"
#endif
#include "epoll_monitor.h"

#include <termios.h>

#ifdef USESSL
#define ListenChannel SSLListenChannel
#define NormalChannel SSLChannel
#else
#define ListenChannel TCPListenChannel
#define NormalChannel TCPChannel
#endif

const std::string IP = "127.0.0.1";
constexpr int PORT = 9999;
constexpr int BUFFER_SIZE = 1024;

bool SetNonCanonicalMode(bool flag = true);

using namespace EasyIO;

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0); // 禁用行缓存，直接输出
    SetNonCanonicalMode();

#ifdef USESSL
    SSLChannel::OpenSSL("key/server/device.crt", "key/server/device.key");
#endif

#ifdef SERVER
    constexpr int MONITOR_CAPACITY = 32;
    auto channel = std::make_unique<ListenChannel>(IP, PORT);
#else
    constexpr int MONITOR_CAPACITY = 1;
    auto channel = std::make_unique<NormalChannel>(IP, PORT);
#endif

    EpollMonitor monitor(MONITOR_CAPACITY);
#ifdef SERVER
    channel->SetCallback([&monitor](std::shared_ptr<Channel> channel) {
#endif
        // 利用闭包机制给通道绑定一个读缓冲区，用于搜索协议分隔符并组包，演示最基础的协议处理方法。
        // 1、适配非阻塞 IO，单次触发不必读完整个协议包； 2、避免逐字节读取判断导致系统调用频繁。
        auto textBuffer = std::make_shared<std::string>();
        channel->SetCallback([&monitor, textBuffer](std::shared_ptr<Channel> self) {
            char data[BUFFER_SIZE + 1];
            int64_t readBytes = self->ReadOnce((uint8_t*)data, BUFFER_SIZE);
            if (readBytes < 0) {
                monitor.RemoveChannel(self->GetHandle());
                std::cout << "[device" << self->GetHandle() << " has been closed...]\n";
                return;
            }
            data[readBytes] = 0;
            textBuffer->append(data);
            size_t index = textBuffer->find_first_of('\n'); // '\n' 作为聊天数据包的分隔符
            if (index != std::string::npos) {
                std::cout << "[device" << self->GetHandle() << "] " <<
                    textBuffer->substr(0, index + 1);
                *textBuffer = textBuffer->substr(index + 1);
            } else {
                // std::cout << "收到部分数据，尚未收到协议分隔符，暂存起来\n";
            }
        });
#ifdef SERVER
        monitor.AddChannel(channel);
    });
#endif
    if (!channel->Initialise()) {
        return 1;
    }
    monitor.AddChannel(std::move(channel));
    monitor.Start();

    char input;
    while (true) {
        std::string text;
        while (input = getchar()) {
            if (input == '~') { // 支持按 '~' 中断，用于模拟分片发送和验证协议组包
                break;
            }
            text.push_back(input);
            if (input == '\n') { // 回车中断时会带上 '\n' 符号本身
                break;
            }
        }
        if (text == "q") {
            break;
        }
        monitor.Write((uint8_t*)text.c_str(), text.length());
    }

    SetNonCanonicalMode(false);
    return 0;
}

// 设置终端的 canonical/non-canonical 模式
// non-canonical 模式下，无须回车即可触发 getchar 读取，同时对输入长度没有 4096 字节限制
bool SetNonCanonicalMode(bool flag)
{
    struct termios settings;
    int result;
    result = tcgetattr(STDIN_FILENO, &settings);
    if (result < 0) {
        return false;
    }

    if (flag) {
        settings.c_lflag &= ~ICANON;
    } else {
        settings.c_lflag |= ICANON;
    }

    result = tcsetattr(STDIN_FILENO, TCSANOW, &settings);
    if (result < 0) {
        return false;
    }
    return true;
}