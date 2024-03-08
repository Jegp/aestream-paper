#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include "aer.hpp"
#include "generator.hpp"
#include "noop.hpp"
#include "result.hpp"
#include "task.hpp"

using namespace std::chrono_literals;
using namespace Task;

class BaseBenchmark
{
public:
    BaseBenchmark(const std::string& name, const TaskType task, const std::string& task_name);

    virtual std::tuple<size_t, size_t> benchmark(
        size_t run,
        size_t runs,
        size_t thread_count,
        std::vector<size_t> thread_counts,
        size_t buffer_size,
        std::vector<size_t> buffer_sizes,
        const std::vector<AER::Event>& events,
        const size_t checksum) = 0;
    void run(const size_t n_runs, const std::vector<AER::Event>& events, const size_t checksum);
    void update(std::vector<Result>&);
    void compute_stats();

    std::vector<Result> result;

protected:
    std::string name{"Benchmark"};
    TaskType task;
    const std::string task_name;
    std::vector<size_t> buffer_sizes;
    std::vector<size_t> thread_counts;

    double mean{0.0};
    double sd{0.0};
    std::vector<size_t> runtimes;
    std::vector<AER::Event> events;
};

#endif // BENCHMARK_HPP
