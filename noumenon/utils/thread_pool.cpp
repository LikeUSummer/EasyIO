#include "thread_pool.h"

void ThreadPool::Worker()
{
    while (true) {
        std::function<void()> task;
        // 有锁环境从队列取任务
        {
            std::unique_lock<std::mutex> lock(lockForTaskQueue_);
            // 如果线程池处于运行状态且任务队列为空，则解锁并等待 Push 的通知，被通知后重新加锁并继续
            // 反之，直接继续
            condition_.wait(lock, [this]() {
                return stop_ || !taskQueue_.empty();
            });
            if (stop_ && taskQueue_.empty()) {
                return;
            }
            task = std::move(taskQueue_.front());
            taskQueue_.pop();
        }
        // 无锁环境执行任务
        task();
    }
}

ThreadPool::ThreadPool(int capacity)
{
    while (capacity--) {
        threadPool_.emplace_back(&ThreadPool::Worker, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(lockForTaskQueue_);
        stop_ = true;
    }
    condition_.notify_all();
    for (auto& thread : threadPool_) {
        thread.join();
    }
}
