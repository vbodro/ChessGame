#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Submit a callable, returns future with its result.
    // Usage: auto fut = pool.submit(fn, arg1, arg2);
    template<typename Fn, typename... Args>
    auto submit(Fn&& fn, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>> {
        using ReturnType = std::invoke_result_t<Fn, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)
        );
        std::future<ReturnType> future = task->get_future();

        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.emplace([task]() { (*task)(); });
        }
        cv.notify_one();
        return future;
    }

private:
    std::vector<std::thread>          workers;
    std::queue<std::function<void()>> tasks;
    std::mutex                        mutex;
    std::condition_variable           cv;
    bool                              stop = false;

    void workerLoop();
};
