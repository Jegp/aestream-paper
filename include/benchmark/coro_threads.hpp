#ifndef CORO_THREADS_BENCHMARK_HPP
#define CORO_THREADS_BENCHMARK_HPP

#include "aer.hpp"
#include "benchmark/base.hpp"
#include "noop.hpp"
#include "task.hpp"
#include <vector>

class CoroThreadBenchmark : public BaseBenchmark
{
public:
    CoroThreadBenchmark(
        const std::string& name,
        const std::vector<size_t> buffer_sizes,
        const std::vector<size_t> thread_counts,
        const TaskType task,
        const std::string& task_name);

    void prepare(const size_t event_count);
    void cleanup();
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

    std::atomic<bool> done{false};
    std::mutex awaiter_list_lock;
    std::vector<std::jthread> threads{};
    std::vector<ThreadPromise<AER::Event>*> awaiters{};
};

#endif // CORO_THREADS_BENCHMARK_HPP
