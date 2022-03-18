#include "ssl_channel.h"
#include "ssl_listen_channel.h"

namespace EasyIO {
bool SSLListenChannel::OnRead()
{
#ifdef NON_BLOCK_CHANNEL
    while (1)
#endif
    {
        int handle = OnConnect();
        if (handle < 0) {
            return false;
        }
        auto channel = std::make_shared<SSLChannel>(handle);
        if (!channel->Initialise()) {
            return false;
        }
        callback_(channel);
    }
    return true;
}
}