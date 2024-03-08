#ifndef CORO_BENCHMARK_HPP
#define CORO_BENCHMARK_HPP

#include "benchmark/base.hpp"
#include "task.hpp"
#include <vector>

class VanillaCoroBenchmark : public BaseBenchmark
{
public:
    VanillaCoroBenchmark(
        const std::string& name,
        const std::vector<size_t> buffer_sizes,
        const std::vector<size_t> thread_counts,
        const TaskType task,
        const std::string& task_name);

    void prepare(const size_t event_count);
    void cleanup();
    CoroTask run_task(const size_t x, const size_t y, Accumulator& acc);
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
};

#endif // CORO_BENCHMARK_HPP
