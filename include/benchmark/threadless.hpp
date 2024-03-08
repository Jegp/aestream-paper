#ifndef THREADLESS_BENCHMARK_HPP
#define THREADLESS_BENCHMARK_HPP

#include "benchmark/base.hpp"
#include <vector>

class ThreadlessBenchmark : public BaseBenchmark
{
public:
    ThreadlessBenchmark(
        const std::string& name,
        const std::vector<size_t> buffer_sizes,
        const std::vector<size_t> thread_counts,
        const TaskType task,
        const std::string& task_name);

    void cleanup();
    void run_task(const size_t& x, const size_t& y, Accumulator& acc);
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
    std::vector<size_t> buffer_sizes;
    std::vector<size_t> thread_counts;
};

#endif // THREADLESS_BENCHMARK_HPP
