#include "http_downloader.h"

using namespace EasyIO;

int main(int argc, char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);

    std::cout << "First, to download the default test item...\n";
    // https://pm.myapp.com/invc/xfspeed/qqpcmgr/download/QQPCDownload1856.exe 腾讯电脑管家
    std::string url = "https://gimg2.baidu.com/image_search/src=http%3A%2F%2Fimg.jj20.com%2Fup%2Fallimg%2Ftp06%2F19121101341355c-0-lp.jpg";
    std::string fileName = "test.jpg";
    std::cout << "[URL] " << url << std::endl;
    std::cout << "[File Name] " << fileName << std::endl;

    Downloader downloader(url, fileName, 8);
    downloader.Download();

    std::cout << "Then, you can input your own url to test...\n";
    while (true) {
        std::string url;
        std::string fileName;
        std::cout << "[URL] ";
        std::cin >> url;
        std::cout << "[File Name] ";
        std::cin >> fileName;
        Downloader downloader(url, fileName, 8);
        downloader.Download();
    }
    return 0;
}