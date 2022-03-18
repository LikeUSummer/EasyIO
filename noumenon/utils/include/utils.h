#include <functional>
#include <string>

namespace EasyIO {
    bool CreateDir(const std::string& dirPath);
    bool ForEachFile(const std::string& dirPath,
        std::function<void (const std::string&)> callback);
}