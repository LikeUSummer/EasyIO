#include "thread_pool.h"
#include <iostream>

void task(char c)
{
    int i = 3;
    while (i--) {
        std::cout << c << ' ';
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    ThreadPool pool(6); // 改变线程池规模以观察运行效果
    for (char c = 'A'; c <= 'Z'; ++c) {
        pool.Push(task, c);
    }
    while (1) {};
    return 0;
}