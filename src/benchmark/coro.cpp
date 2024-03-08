#include "benchmark/coro.hpp"
#include "task.hpp"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

VanillaCoroBenchmark::VanillaCoroBenchmark(
    const std::string& name,
    const std::vector<size_t> buffer_sizes,
    const std::vector<size_t> thread_counts,
    const TaskType task,
    const std::string& task_name)
    : BaseBenchmark(name, task, task_name), buffer_sizes(buffer_sizes), thread_counts(thread_counts)
{
}

CoroTask VanillaCoroBenchmark::run_task(const size_t x, const size_t y, Accumulator& acc)
{
}

void VanillaCoroBenchmark::benchmark(
    size_t run,
    size_t runs,
    size_t thread_count,
    std::vector<size_t> thread_counts,
    size_t buffer_size,
    std::vector<size_t> buffer_sizes,
    const std::vector<AER::Event>& events,
    const size_t checksum){};

void VanillaCoroBenchmark::cleanup()
{
}
