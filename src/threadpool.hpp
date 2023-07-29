#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <syncstream>
#include <thread>
#include <vector>

/// Rudimentary debug printing.
template <typename... Args> std::array<int, sizeof...(Args)> log(Args &&..._args)
{
  static std::mutex out_mtx;

  std::lock_guard lk{ out_mtx };
  std::array<int, sizeof...(_args)> status{
      (std::cout << std::forward<Args>(_args), 0)... };
  std::cout << '\n';
  return status;
}

namespace Async
{
  using uint = unsigned long;
  using auint = std::atomic<uint>;
  using flag = std::atomic<bool>;
  using lguard = std::lock_guard<std::mutex>;
  template <class T> using uptr = std::unique_ptr<T>;

  struct TaskBase
  {
    virtual ~TaskBase() {};
    virtual uint operator()() = 0;
  };

  using Task = uptr<TaskBase>;

  template <typename F, typename... Args>
  struct TaskExecutor: TaskBase
  {
    std::function<uint(Args &&...)> _func;
    std::tuple<Args &&...> _args;

    constexpr TaskExecutor(F&& func, Args &&...args)
      : _func(std::forward<F>(func)), _args(std::forward<Args>(args)...)
    {
    }

    ~TaskExecutor() {};

    uint operator()() override final { return std::apply(_func, _args); }
  };

  ///=============================================================================
  ///	Main feature
  ///=============================================================================

  /// @class Threadpool
  class ThreadPool
  {

  private:
    /// Task queue.
    std::vector<Task> buffer;

    // Number of workers
    uint worker_count{ std::thread::hardware_concurrency() };

    // Buffer size
    uint buffer_size{ 1024 };

    /// Mutex for the lock guards.
    std::mutex mtx;

    // Flag for signalling threads that they should quit.
    flag halt{ false };

    // A container for all the worker threads.
    std::vector<std::jthread> threads;

    // Worker counter.
    // Plays a role in the destructor.
    auint workers{ 0 };

  public:
    // Total sum for all executions
    auint total_sum{ 0 };

    ThreadPool(
      const uint worker_count = std::thread::hardware_concurrency(),
      const uint buffer_size = 1024)
      :
      worker_count{ worker_count },
      buffer_size{ buffer_size }
    {
      buffer.reserve(buffer_size);

      for (uint i = 0; i < worker_count; ++i)
      {
        threads.emplace_back(add_worker());
      }
    }

    ~ThreadPool() noexcept
    {
      // halt.store(true);

      // while (workers.load() > 0);

      // log("Threadpool closing with total sum: ", total_sum.load());

    }

    template <typename F, typename... Args>
    void enqueue(F&& fun, Args &&...args)
    {
      const lguard lg{ mtx };
      buffer.emplace_back(
        make_task(std::forward<F>(fun), std::forward<Args>(args)...));
      //                log("Enqueue | buffer size ", buffer.size());
    }

    void wait()
    {
      while (workers.load() != worker_count);
    }

    void stop()
    {
      if (halt.load())
      {
        // log("Threadpool already stopped.");
        return;
      }

      // log("Threadpool stopping...");

      {

        lguard lg{ mtx };

        // Inform the workers that they should stop.
        halt.store(true);

        // Empty the queue
        buffer.clear();
      }
    }

    void sync()
    {
      if (halt.load())
      {
        // log("Threadpool already stopped.");
        return;
      }

      // {

      //   lguard lg{ mtx };
      //   // log("Threadpool stopping...");

        // Inform the workers that they should stop.
        halt.store(true);
      // }

      while (workers.load() > 0);

      // log("Threadpool closing with total sum: ", total_sum.load());

    }


  private:
    template <typename Func, typename... Args>
    Task make_task(Func&& fun, Args &&...args)
    {
      // using Ret = typename std::invoke_result<Func, Args...>::type;
      return std::make_unique<TaskExecutor<Func, Args...>>(
        std::forward<Func>(fun), std::forward<Args>(args)...);
    }

    std::jthread add_worker()
    {
      return std::jthread([&]() mutable
        {
          ++workers;

          std::vector<Task> _buffer;

          _buffer.reserve(buffer_size);

          uint sum{ 0 };

          while (true)
          {

            {
              const lguard lg(mtx);
              //                            log("Swapping | _buffer size: ",
              //                            _buffer.size(), " | buffer size: ",
              //                            buffer.size());
              //
              if (halt.load() && buffer.empty())
              {
                break;
              }
              _buffer.swap(buffer);
            }

            for (uint i = 0; i < _buffer.size(); ++i)
            {
              // Execute the task
              sum += (*_buffer[i])();
            }
            _buffer.clear();
          }
          // log("Finished | sum: ", sum);

          // {
          //   const lguard lg(mtx);
            total_sum.fetch_add(sum);
            --workers;
          // }
        });
    }
  };
} // namespace Async
#endif // THREADPOOL_HPP
