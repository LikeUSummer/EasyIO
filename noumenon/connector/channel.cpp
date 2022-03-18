#include "channel.h"

namespace EasyIO {
bool Channel::OnRead()
{
    if (callback_) {
        callback_(shared_from_this());
    }
    return true;
}
}