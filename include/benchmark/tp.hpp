#ifndef THREADPOOL_BENCHMARK_HPP
#define THREADPOOL_BENCHMARK_HPP

#include "benchmark/base.hpp"
#include "task.hpp"
#include "threadpool.hpp"
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

    void cleanup();
    CoroTask run_task(const size_t& x, const size_t& y, Accumulator& acc);
    std::tuple<size_t, size_t> benchmark(
        size_t run,
        size_t runs,
        size_t thread_count,
        std::vector<size_t> thread_counts,
        size_t buffer_size,
        std::vector<size_t> buffer_sizes,
        const std::vector<AER::Event>& events,
        const size_t checksum) override;

private:
    uptr<ThreadPool> tp{nullptr};
    std::vector<size_t> buffer_sizes;
    std::vector<size_t> thread_counts;
};

#endif // THREADPOOL_BENCHMARK_HPP
