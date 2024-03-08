#include "benchmark/coro_threads.hpp"
#include "task.hpp"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

CoroThreadBenchmark::CoroThreadBenchmark(
    const std::string& name,
    const std::vector<size_t> buffer_sizes,
    const std::vector<size_t> thread_counts,
    const TaskType task,
    const std::string& task_name)
    : BaseBenchmark(name, task, task_name), buffer_sizes(buffer_sizes), thread_counts(thread_counts)
{
}

std::tuple<size_t, size_t> CoroThreadBenchmark::benchmark(
    size_t run,
    size_t runs,
    size_t thread_count,
    std::vector<size_t> thread_counts,
    size_t buffer_size,
    std::vector<size_t> buffer_sizes,
    const std::vector<AER::Event>& events,
    const size_t checksum)
{

    for (size_t i = 0; i < thread_count; ++i)
    {
        threads.push_back(std::jthread(
            [&]() mutable { thread_runner<AER::Event>(awaiters, awaiter_list_lock, done); }));
    }
    while (awaiters.size() < thread_count)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Run benchmark
    AtomicAccumulator acc{};
    auto sum_co_lambda                = [&](AER::Event e) { task(e.x, e.y, acc); };
    ReturnPromise<AER::Event>* object = co_await GetPromise<ReturnPromise<AER::Event>>{};
    object->awaiters                  = awaiters;
    object->set_callback(sum_co_lambda);

    // std::cout << "Awaiters " << object->awaiters.size() << std::endl;

    // Run
    auto before = std::chrono::high_resolution_clock::now();
    for (auto event : events)
    {
        co_yield event;
    }
    std::this_thread::sleep_for(std::chrono::nanoseconds(events.size() / 1000));
    auto after    = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();

    cleanup_coroutines<AER::Event>(threads, awaiters, done);

    return std::make_tuple(acc.get(), duration);
};

void CoroThreadBenchmark::cleanup()
{
}
