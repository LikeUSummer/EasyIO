#include "utils.h"

#include <cstring>
#include <iostream>

#include <sys/stat.h>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

namespace EasyIO {
bool CreateDir(const std::string& dirPath)
{
    int length = dirPath.length();
    for (int i = 0; i < length; ++i) {
        if (dirPath[i] == '/' || i == length - 1) {
            std::string path = dirPath.substr(0, i + 1);
            if (access(path.c_str(), F_OK) == 0) {
                continue;
            }
            if (mkdir(path.c_str(), 0777) < 0) {
                std::cout << "mkdir failed: " << path << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool ForEachFile(const std::string& dirPath,
    std::function<void (const std::string&)> callback)
{
    struct dirent* stdir;
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        std::cout << "opendir failed: " << dirPath << std::endl;
        return false;
    }
    while (stdir = readdir(dir)) {
        if(strcmp(stdir->d_name, ".") == 0 || strcmp(stdir->d_name, "..") == 0) {
            continue;
        }
        if (stdir->d_type == 8) {
            callback(dirPath + stdir->d_name);
        }
        // else if(stdir->d_type == 4) { // visit sub directories recursively
        //     ForEachFile(dirPath + stdir->d_name + "/", callback);
        // }
    }
    closedir(dir);
    return true;
}
}