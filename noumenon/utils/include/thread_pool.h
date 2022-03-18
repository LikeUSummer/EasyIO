#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

class ThreadPool {
public:
    ThreadPool(int);
    template<class F, class... Args>
    auto Push(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();

private:
    std::vector<std::thread> threadPool_;
    std::queue<std::function<void()>> taskQueue_;

    std::mutex lockForTaskQueue_;
    std::condition_variable condition_;
    bool stop_ {false};

private:
    void Worker();
};

template<class F, class... Args>
auto ThreadPool::Push(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<return_type> result = task->get_future();
    // 加锁入队
    {
        std::unique_lock<std::mutex> lock(lockForTaskQueue_);
        if (stop_) {
            throw std::runtime_error("thread pool has already stopped");
        }
        taskQueue_.emplace([task]() {
            (*task)();
        });
    }
    condition_.notify_one();
    return result;
}
#endif
