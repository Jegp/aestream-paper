#ifndef THREADPOOL_CORO_BENCHMARK_HPP
#define THREADPOOL_CORO_BENCHMARK_HPP

#include "benchmark/base.hpp"
#include "threadpool_coro.hpp"
#include <vector>

using namespace Async;

class ThreadPoolBenchmark : public BaseBenchmark
{
public:
    ThreadPoolBenchmark(
        const std::string& name,
        const std::vector<size_t> buffer_sizes,
        const std::vector<size_t> thread_counts,
        const TaskType task,
        const std::string& task_name);

    void prepare(const size_t event_count);
    void cleanup();
    CoroTask run_task(const size_t x, const size_t y, Accumulator& acc);
    void benchmark(
        const size_t n_runs,
        const std::vector<AER::Event>& events,
        const size_t checksum) override;

private:
    uptr<ThreadPool> tp{nullptr};
    std::vector<size_t> buffer_sizes;
    std::vector<size_t> thread_counts;
};

#endif // THREADPOOL_CORO_BENCHMARK_HPP