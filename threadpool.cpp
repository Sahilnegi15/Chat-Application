#include "threadpool.h"

ThreadPool::ThreadPool(int numThreads) : m_threadCount(numThreads), stop(false) {
    for (int i = 0; i < m_threadCount; ++i) {
        threads.emplace_back([this] {
            while (true) {
                function<void()> task;

                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this] {
                    return !tasks.empty() || stop;
                });

                if (stop && tasks.empty())
                    return;

                task = move(tasks.front());
                tasks.pop();
                lock.unlock();

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        unique_lock<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}

// Note: Template functions must remain in header file (threadpool.h)
// because C++ template code must be visible to compiler during compilation.
