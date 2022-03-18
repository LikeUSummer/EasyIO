// 测试无锁环队列
#include "ring_queue.h"

#include <chrono>
#include <functional>
#include <random>
#include <thread>

uint32_t g_seed = 0;

struct Data {
    uint64_t stu_id;
    uint32_t age;
    uint32_t score;
};

void Print(const Data* data)
{
    printf("id:%lu\t", data->stu_id);
    printf("age:%u\t", data->age);
    printf("score:%u\n", data->score);
}

Data GenData()
{
    Data data;
    static int id = 0;
    std::default_random_engine gen(g_seed);
    std::uniform_int_distribution<int> dis1(18,25);
    std::uniform_int_distribution<int> dis2(60,100);
    data.stu_id = ++id;
    data.age = dis1(gen);
    data.score = dis2(gen);
    return data;
}

void Consumer(RingQueue& ringQueue)
{
    Data data;
    while(1) {
        std::default_random_engine gen(g_seed);
        std::uniform_int_distribution<int> dis(200, 3000);
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));

        while(!ringQueue.Front((uint8_t*)&data, sizeof(Data))) {
            printf("~~~~~~~~~~~~~~~Wait to Get~~~~~~~~~~~~~~~~\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        ringQueue.Pop(sizeof(Data));
        printf("----------------Get Done------------------\n");
        Print(&data);
        printf("Buffer Used: %u\n", ringQueue.GetSize());
    }
}

void Producer(RingQueue& ringQueue)
{
    while(1) {
        Data data = GenData();
        while (!ringQueue.Push((uint8_t*)&data, sizeof(Data))) {
            printf("~~~~~~~~~~~~~~~Wait to Put~~~~~~~~~~~~~~~~\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        printf("****************Put Done******************\n");
        Print(&data);
        printf("Buffer Capacity: %u\n", ringQueue.GetCapacity());
    
        std::default_random_engine gen(g_seed);
        std::uniform_int_distribution<int> dis(500, 2000);
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
    }
}

int main()
{
    RingQueue ringQueue;
    std::thread t1(Producer, std::ref(ringQueue));
    std::thread t2(Consumer, std::ref(ringQueue));

    while (1) {
        g_seed = std::chrono::system_clock::now().time_since_epoch().count();
    }
    t1.join();
    t2.join();
    return 0;
}
