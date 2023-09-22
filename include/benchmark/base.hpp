#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include "aer.hpp"
#include "generator.hpp"
#include "noop.hpp"
#include "result.hpp"
#include "task.hpp"
#include "threadpool.hpp"

using namespace std::chrono_literals;
using namespace Async;
using namespace Task;

class BaseBenchmark
{
public:
    BaseBenchmark(const std::string& name,
                   const TaskType task, const std::string task_name, const size_t checksum);

    virtual void benchmark(const size_t n_runs, const std::vector<AER::Event>& events) = 0;
    void run(const size_t count, const std::vector<AER::Event>& events);
    void compute_stats();

    std::vector<Result> results;
protected:
    std::string name{ "Benchmark" };
    TaskType task;
    std::string task_name;

    const size_t checksum;
    double mean{ 0.0 };
    double sd{ 0.0 };
    size_t output{ 0 };
    std::vector<size_t> runtimes;
};

#endif // BENCHMARK_HPP
