#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <barrier>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <syncstream>
#include <thread>
#include <utility>
#include <vector>

static auto scout = []() { return std::osyncstream(std::cout); };

namespace Async

{
using uint = unsigned long long;
using auint = std::atomic<uint>;
template <class T> using uptr = std::unique_ptr<T>;
using lguard = std::lock_guard<std::mutex>;
struct CoroTask {
  struct promise_type {
    CoroTask get_return_object() noexcept {
      return CoroTask{std::coroutine_handle<promise_type>::from_promise(*this)};
    };

    std::suspend_never initial_suspend() const noexcept { return {}; }
    std::suspend_never final_suspend() const noexcept { return {}; }

    void return_void() noexcept {}

    void unhandled_exception() noexcept {
      std::cerr << "Unhandled exception caught...\n";
      exit(1);
    }
  };

  explicit CoroTask(std::coroutine_handle<promise_type> handle)
      : m_handle(handle) {}

  ~CoroTask() {
    if (m_handle.done()) {
      m_handle.destroy();
    }
  }

private:
  std::coroutine_handle<promise_type> m_handle;
};

/// @class Threadpool
class ThreadPool {

private:
  /// Coroutine handle queue.
  std::vector<std::coroutine_handle<>> handles;

  /// Mutex for the lock guards.
  std::mutex mtx;

  // Flag for signalling threads that they should quit.
  bool halt{false};

  // Flag for signalling threads that they should synchronise.
  bool synchronise{false};

  // A container for all the worker threads.
  std::vector<std::jthread> workers;

  // A barrier for synchronisation purposes
  std::barrier<> barrier;

public:
  ThreadPool(const uint worker_count = std::thread::hardware_concurrency(),
             const uint buffer_size = 1024)
      : barrier{static_cast<std::ptrdiff_t>(worker_count + 1)} {
    handles.reserve(buffer_size);

    for (uint i = 0; i < worker_count; ++i) {
      workers.emplace_back(add_worker());
    }

    scout() << "Waiting for workers...\n";
    barrier.arrive_and_wait();
    scout() << "Threadpool running...\n";
  }

  ~ThreadPool() noexcept {
    scout() << "Threadpool stopping...\n";
    halt = true;
    scout() << "Waiting for workers...\n";
    barrier.arrive_and_wait();
    scout() << "Threadpool exiting...\n";
  }

  void enqueue(std::coroutine_handle<> handle) {
    lguard lg{mtx};
    handles.emplace_back(handle);
  }

  auto schedule() { return awaiter{*this}; }

  void stop() {
    const lguard lg{mtx};

    // Inform the workers that they should stop.
    halt = true;

    // Empty the queue
    handles.clear();
  }

  void sync() {

    synchronise = true;
    barrier.arrive_and_wait();
    synchronise = false;
  }

private:
  struct awaiter {
    ThreadPool &tp;

    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_resume() const noexcept {}

    // std::coroutine_handle<>
    void await_suspend(std::coroutine_handle<> handle) const noexcept {
      tp.enqueue(handle);
      //   return std::noop_coroutine();
    }
  };
  std::jthread add_worker() {
    return std::jthread([&]() mutable {
      std::vector<std::coroutine_handle<>> buffer;

      barrier.arrive_and_wait();
      while (!halt) {
        {
          const lguard lg(mtx);
          buffer.swap(handles);
        }

        // scout() << "Thread '" << std::this_thread::get_id() << "' resuming "
        //         << buffer.size() << " coroutines\n";
        for (auto handle : buffer) {
          handle.resume();
        }
        buffer.clear();
        if (synchronise) {
          barrier.arrive_and_wait();
        }
      }

      barrier.arrive_and_wait();
    });
  }
};
} // namespace Async
#endif // THREADPOOL_HPP
