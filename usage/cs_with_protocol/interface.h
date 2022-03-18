#include "channel.h"

namespace EasyIO {
bool PushFile(std::shared_ptr<Channel> channel, const std::string& filePath, const std::string& type);
bool PushEvent(std::shared_ptr<Channel> channel, const std::string& event);
void PullFile(std::shared_ptr<Channel> channel, const std::string& type);
void IntroduceMyself(std::shared_ptr<Channel> channel);
}