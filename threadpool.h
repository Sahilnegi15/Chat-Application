#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
using namespace std;

class ThreadPool {
private:
    int m_threadCount;
    vector<thread> threads;
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool stop;

public:
    explicit ThreadPool(int numThreads);
    ~ThreadPool();

    template <class F, class... Args>
    auto ExecuteTask(F&& f, Args&&... args) -> future<decltype(f(args...))> {
        using return_type = decltype(f(args...));

        auto task = make_shared<packaged_task<return_type()>>(
            bind(forward<F>(f), forward<Args>(args)...)
        );

        future<return_type> res = task->get_future();
        {
            unique_lock<mutex> lock(mtx);
            tasks.push([task]() { (*task)(); });
        }
        cv.notify_one();
        return res;
    }
};

#endif // THREADPOOL_H
