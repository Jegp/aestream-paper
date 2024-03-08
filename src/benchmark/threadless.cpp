#include "benchmark/threadless.hpp"
#include "task.hpp"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

ThreadlessBenchmark::ThreadlessBenchmark(
    const std::string& name,
    const std::vector<size_t> buffer_sizes,
    const std::vector<size_t> thread_counts,
    const TaskType task,
    const std::string& task_name)
    : BaseBenchmark(name, task, task_name), buffer_sizes(buffer_sizes), thread_counts(thread_counts)
{
}

void ThreadlessBenchmark::run_task(const size_t& x, const size_t& y, Accumulator& acc)
{
    task(x, y, acc);
}

// Main feature
////////////////////////////////////////
std::tuple<size_t, size_t> ThreadlessBenchmark::benchmark(
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
    SimpleAccumulator acc{};

    auto before{std::chrono::high_resolution_clock::now()};

    // Main feature
    ////////////////////////////////////////
    for (const auto& event : events)
    {
        run_task(event.x, event.y, acc);
    }

    auto after{std::chrono::high_resolution_clock::now()};
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();
    return std::make_tuple(acc.get(), duration);
};

void ThreadlessBenchmark::cleanup()
{
}
