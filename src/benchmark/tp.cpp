#include "benchmark/tp.hpp"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

using namespace Async;

ThreadPoolBenchmark::ThreadPoolBenchmark(
    const std::string& name,
    const std::vector<size_t> buffer_sizes,
    const std::vector<size_t> thread_counts,
    const TaskType task,
    const std::string& task_name)
    : BaseBenchmark(name, task, task_name), buffer_sizes(buffer_sizes), thread_counts(thread_counts)
{
}

CoroTask ThreadPoolBenchmark::run_task(const size_t& x, const size_t& y, Accumulator& acc)
{
    //   scout() << "Before schedule: " << std::this_thread::get_id() << "\n";
    co_await tp->schedule();
    //   scout() << "After schedule: " << std::this_thread::get_id() << "\n";
    task(x, y, acc);
}

std::tuple<size_t, size_t> ThreadPoolBenchmark::benchmark(
    size_t run,
    size_t runs,
    size_t thread_count,
    std::vector<size_t> thread_counts,
    size_t buffer_size,
    std::vector<size_t> buffer_sizes,
    const std::vector<AER::Event>& events,
    const size_t checksum)
{

    // An accumulator to hold the checksum.
    AtomicAccumulator acc{};

    // Create the threadpool before starting the timer.
    tp = std::make_unique<ThreadPool>(thread_count, buffer_size);

    auto before{std::chrono::high_resolution_clock::now()};

    // Main feature
    ////////////////////////////////////////
    for (const auto& event : events)
    {
        run_task(event.x, event.y, acc);
    }

    tp = nullptr;

    auto after{std::chrono::high_resolution_clock::now()};
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();
    return std::make_tuple(acc.get(), duration);
};

void ThreadPoolBenchmark::cleanup()
{
}
