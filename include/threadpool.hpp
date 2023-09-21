#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <barrier>
#include <syncstream>
#include <iostream>
#include <coroutine>

static auto scout = []() { return std::osyncstream(std::cout); };

namespace Async

{
    using uint = unsigned long long;
    using auint = std::atomic<uint>;
    template <class T>
    using uptr = std::unique_ptr<T>;
    using lguard = std::lock_guard<std::mutex>;

    /// @class Threadpool
    class ThreadPool
    {

    private:
        /// Coroutine handle queue.
        std::vector<std::coroutine_handle<>> handles;

        /// Mutex for the lock guards.
        std::mutex mtx;

        // Flag for signalling threads that they should quit.
        bool halt{ false };

        // A container for all the worker threads.
        std::vector<std::jthread> workers;

        // A barrier for synchronisation purposes
        std::barrier<> barrier;


    public:


        ThreadPool(const uint worker_count = std::thread::hardware_concurrency(),
                   const uint buffer_size = 1024)
            :
            barrier{ static_cast<std::ptrdiff_t>(worker_count + 1) }
        {
            handles.reserve(buffer_size);

            for (uint i = 0; i < worker_count; ++i)
            {
                workers.emplace_back(add_worker());
            }

            scout() << "Waiting for workers...\n";
            barrier.arrive_and_wait();
            scout() << "Threadpool running...\n";
        }

        ~ThreadPool() noexcept
        {
            scout() << "Threadpool stopping...\n";
            halt = true;
            scout() << "Waiting for workers...\n";
            barrier.arrive_and_drop();
            scout() << "Threadpool exiting...\n";
        }

        void enqueue(std::coroutine_handle<>&& handle)
        {
            const lguard lg{ mtx };
            handles.emplace_back(handle);
        }

        auto schedule()
        {
            return awaiter{ *this };
        }

        void stop()
        {
            const lguard lg{ mtx };

            // Inform the workers that they should stop.
            halt = true;

            // Empty the queue
            handles.clear();
        }

        void sync()
        {
            if (halt)
            {
                return;
            }
            barrier.arrive_and_wait();
        }

    private:

        struct awaiter
        {
            ThreadPool& tp;

            constexpr bool await_ready() const noexcept { return false; }
            constexpr void await_resume() const noexcept {}
            void await_suspend(std::coroutine_handle<>&& handle) const noexcept
            {
                tp.enqueue(std::forward<std::coroutine_handle<>>(handle));
            }
        };

        std::jthread add_worker()
        {
            return std::jthread([&]() mutable
            {

                std::vector<std::coroutine_handle<>> buffer;

                barrier.arrive_and_wait();
                while (!halt)
                {

                    {
                        const lguard lg(mtx);
                        buffer.swap(handles);
                    }

                    for (auto handle : buffer)
                    {
                        handle.resume();
                        handle.destroy();
                    }
                    buffer.clear();
                }

                barrier.arrive_and_drop();
            });
        }
    };
} // namespace Async
#endif // THREADPOOL_HPP
