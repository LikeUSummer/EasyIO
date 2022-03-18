#include "ssl_channel.h"

namespace EasyIO {
SSL_CTX* SSLChannel::sslContext_ = NULL;

SSLChannel::~SSLChannel()
{
    if (sslHandle_) {
        SSL_free(sslHandle_);
    }
}

bool SSLChannel::OpenSSL(const std::string& crtPath, const std::string& keyPath)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    // 初始化环境
    sslContext_ = SSL_CTX_new(TLS_method());
    if (sslContext_ == NULL) {
        std::cout << ERR_error_string(ERR_get_error(), NULL);
        return false;
    }
    // 指定使用 TLS 1.3 协议版本
    SSL_CTX_set_min_proto_version(sslContext_, TLS1_3_VERSION);
    // SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    // 加载证书文件（含公钥），用于发送给对端
    if (SSL_CTX_use_certificate_file(sslContext_, crtPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cout << ERR_error_string(ERR_get_error(), NULL);
        return false;
    }
    // 不校验 CA 签名，即从对端证书中直接取出公钥
    SSL_CTX_set_verify(sslContext_, SSL_VERIFY_NONE, NULL);
    /*
    // 指定 CA 证书文件或目录，向对端发送其摘要信息，对端在本地查找对应 CA 证书，然后校验二级证书签名
    SSL_CTX_set_verify(sslContext_, SSL_VERIFY_PEER, NULL);
    SSL_CTX_load_verify_locations(sslContext_, CAFile, CAPath); 
    */

    // 指定私钥文件的加密密码
    // char key[] = "EasyIO";
    // SSL_CTX_set_default_passwd_cb_userdata(sslContext_, key);
    // 加载私钥文件
    if (SSL_CTX_use_PrivateKey_file(sslContext_, keyPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cout << ERR_error_string(ERR_get_error(), NULL);
        return false;
    }
    // 检验私钥
    if (SSL_CTX_check_private_key(sslContext_) <= 0) {
        std::cout << ERR_error_string(ERR_get_error(), NULL);
        return false;
    }

    // 其他开关
#ifndef NON_BLOCK_CHANNEL
    SSL_CTX_set_mode(sslContext_, SSL_MODE_AUTO_RETRY); // 在重协商时，读写接口内部自动完成重试逻辑，并设置 retry 标记，仅用于阻塞 IO
#endif
    // SSL_CTX_set_mode(sslContext_, SSL_MODE_ENABLE_PARTIAL_WRITE); // 设置允许 SSL_write 只交付部分数据，否则为完整交付模式
    SSL_CTX_set_options(sslContext_, SSL_OP_ALL | SSL_OP_NO_COMPRESSION); // 避开已知漏洞，TLS 1.3 默认已禁用压缩
    SSL_CTX_set_options(sslContext_, SSL_OP_NO_RENEGOTIATION); // 禁用重协商，TLS 1.3 默认已禁用重协商
    return true;
}

void SSLChannel::CloseSSL()
{
    if (sslContext_) {
        SSL_CTX_free(sslContext_);
        sslContext_ = NULL;
    }
}

bool SSLChannel::Initialise()
{
    int err {0};
    sslHandle_ = SSL_new(sslContext_);
    if (handle_ == -1) { // 作为客户端
        if (!TCPChannel::Initialise()) {
            SSL_free(sslHandle_);
            sslHandle_ = NULL;
            return false;
        }
        SSL_set_connect_state(sslHandle_);
    } else { // 作为服务端
        SSL_set_accept_state(sslHandle_);
    }
    SSL_set_fd(sslHandle_, handle_);

#ifdef NON_BLOCK_CHANNEL
    while (true) { // SSL_is_init_finished(sslHandle_)
#endif
        int ret = SSL_do_handshake(sslHandle_); // SSL 握手，执行前要明确 CS 角色（SSL_set_xxx_state）
        if (ret == 1) {
            // std::cout << "SSL handshake successful\n";
            return true;
        }
        err = SSL_get_error(sslHandle_, ret);
#ifdef NON_BLOCK_CHANNEL
        if (err != SSL_ERROR_WANT_WRITE &&
            err != SSL_ERROR_WANT_READ) {
            break;
        }
        usleep(1000);
    }
#endif
    std::cout << "SSL handshake failed\n";
    std::cout << ERR_error_string(ERR_get_error(), NULL);
    SSL_free(sslHandle_);
    sslHandle_ = NULL;
    return false;
}

int64_t SSLChannel::ReadOnce(uint8_t* data, size_t size)
{
    if (size == 0) {
        return 0;
    }
    int err = SSL_ERROR_WANT_WRITE;
    while (err == SSL_ERROR_WANT_WRITE) {
        int r = SSL_read(sslHandle_, data, size);
        if (r > 0) {
            return r;
        } else {
            err = SSL_get_error(sslHandle_, r);
            if (err == SSL_ERROR_WANT_READ) {
                return 0;
            }
        }
        usleep(WAIT_TIME_US);
    }
    return -1;
}

int64_t SSLChannel::WriteOnce(uint8_t* data, size_t size)
{
    if (size == 0) {
        return 0;
    }
    int err = SSL_ERROR_WANT_READ;
    while (err == SSL_ERROR_WANT_READ) {
        int r = SSL_write(sslHandle_, data, size);
        if (r > 0) {
            return r;
        } else {
            err = SSL_get_error(sslHandle_, r);
            if (err == SSL_ERROR_WANT_WRITE) {
                return 0;
            }
        }
        usleep(WAIT_TIME_US);
    }
    return -1;
}
}