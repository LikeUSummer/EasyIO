/*
 * linux环境下安装开发包：apt-get install libssl-dev
 * 编译时指定依赖库： -lssl -lcrypto
 */
#ifndef SSL_CHANNEL_H
#define SSL_CHANNEL_H

#include "tcp_channel.h"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

namespace EasyIO {
class SSLChannel : public TCPChannel {
protected:
    static SSL_CTX* sslContext_;

public:
    SSL* sslHandle_ {NULL};
    // 由服务端构造
    SSLChannel(int handle)
        : TCPChannel(handle) {}
    // 由客户端构造
    SSLChannel(const std::string& ip, int port)
        : TCPChannel(ip, port) {}
    virtual ~SSLChannel();

public:
    // 初始化 SSL 基础设施，每个进程调用一次
    static bool OpenSSL(const std::string& crtPath, const std::string& keyPath);
    static void CloseSSL();

public:
    int64_t ReadOnce(uint8_t* data, size_t size) override;
    int64_t WriteOnce(uint8_t* data, size_t size) override;
    // int64_t ReadRetry(uint8_t* data, size_t size) override;
    // int64_t WriteRetry(uint8_t* data, size_t size) override;
    bool Initialise() override;
};
}
#endif