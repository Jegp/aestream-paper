#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <barrier>
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <syncstream>
#include <thread>
#include <utility>
#include <vector>

static auto scout = []()
{ return std::osyncstream( std::cout ); };

namespace Async

{
    using uint = unsigned long long;
    using auint = std::atomic<uint>;
    template <class T>
    using uptr = std::unique_ptr<T>;
    using lguard = std::lock_guard<std::mutex>;
    using ulock = std::unique_lock<std::mutex>;
    struct CoroTask
    {
        struct promise_type
        {
            CoroTask get_return_object() noexcept
            {
                return CoroTask{ std::coroutine_handle<promise_type>::from_promise( *this ) };
            };

            std::suspend_never initial_suspend() const noexcept
            {
                return {};
            }
            std::suspend_never final_suspend() const noexcept
            {
                return {};
            }

            void unhandled_exception() noexcept
            {
                std::cerr << "Unhandled exception caught...\n";
                exit( 1 );
            }
        };

        explicit CoroTask( std::coroutine_handle<promise_type> handle )
            : m_handle( handle )
        {
        }

      private:
        std::coroutine_handle<promise_type> m_handle;
    };

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

        // Flag for signalling threads that they should synchronise.
        bool synchronise{ false };

        // A container for all the worker threads.
        std::vector<std::jthread> workers;

        // Explicitly store the worker count
        uint worker_count{ 0 };

        // A rudimentary synchronisation point
        struct {
            auint start;
            auint end;

        } sync;

        // Condition variable for notifications
        std::condition_variable cv;

      public:
        ThreadPool( const uint worker_count = std::thread::hardware_concurrency(),
                    const uint buffer_size = 1024 )
            : worker_count{ worker_count }, sync{ 0, 0 }
        {
            handles.reserve( buffer_size );

            for ( uint i = 0; i < worker_count; ++i )
            {
                workers.emplace_back( add_worker() );
            }

            // scout() << "Waiting for workers...\n";
            while (sync.start.load() < worker_count);
            // scout() << "Threadpool running...\n";
        }

        ~ThreadPool() noexcept
        {

            // scout() << "Threadpool stopping...\n";
            stop();
            // scout() << "Waiting for workers...\n";
//             scout() << "Threadpool exiting...\n";
        }

        void enqueue( std::coroutine_handle<> handle )
        {
            {
                lguard lk{ mtx };
                handles.emplace_back( handle );
            }
//            std::cout << "Notifying...\n";
            cv.notify_one();
        }

        auto schedule()
        {
            return awaiter{ *this };
        }

        void stop()
        {
            // Inform the workers that they should stop.
            halt = true;
            cv.notify_all();
            while (sync.end.load() < worker_count);
        }

      private:
        struct awaiter
        {
            ThreadPool& tp;

            constexpr bool await_ready() const noexcept
            {
                return false;
            }
            constexpr void await_resume() const noexcept
            {
            }

            std::coroutine_handle<>
            await_suspend( std::coroutine_handle<> handle ) const noexcept
            {
                tp.enqueue( handle );
                return std::noop_coroutine();
            }
        };

        std::jthread add_worker()
        {
            return std::jthread( [&]() mutable
                                 {
      std::vector<std::coroutine_handle<>> buffer;

      ++sync.start;
      while (true) {
        {
            ulock lk(mtx);
            if (halt && handles.empty())
            {
                break;
            }
            cv.wait(lk, [&](){
                return halt || !handles.empty();
            });
            buffer.swap(handles);
//            std::cout << "Resuming...\n";
        }

        for (auto handle : buffer) {
            handle.resume();
            if (handle.done()) {
                handle.destroy();
            }
        }
        buffer.clear();
      }

      ++sync.end; } );
        }
    };
} // namespace Async
#endif // THREADPOOL_HPP
