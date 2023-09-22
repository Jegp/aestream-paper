#include "benchmark/tp.hpp"
#include "threadpool.hpp"
#include <cstddef>
#include <fstream>

using namespace Async;

ThreadPoolBenchmark::ThreadPoolBenchmark(const std::string& name,
                                         const std::vector<size_t> buffer_sizes,
                                         const std::vector<size_t> thread_counts,
                                         const TaskType task, const std::string task_name, const size_t checksum)
    :
    BaseBenchmark(name, task, task_name, checksum), buffer_sizes(buffer_sizes), thread_counts(thread_counts)
{}

void ThreadPoolBenchmark::prepare(const size_t n_threads,
                                  const size_t n_buffer_size)
{
    tp = std::make_unique<ThreadPool>();
}

void ThreadPoolBenchmark::benchmark(const size_t n_runs, const std::vector<AER::Event>& events)
{
    std::cout << "Running " << buffer_sizes.size() << std::endl;
    for (size_t buffer_size : buffer_sizes)
    {
        for (size_t n_threads : thread_counts)
        {
            prepare(n_threads, buffer_size);

            for (size_t i = 0; i < n_runs; ++i)
            {
                std::cout << "Running " << name << " with " << n_threads << " threads and buffer size " << buffer_size << std::endl;
                // An atomic to hold the checksum
                auto acc = SimpleAccumulator();

                auto before = std::chrono::high_resolution_clock::now();
                for (const auto& event : events)
                {
                    tp->schedule();
                    task(event.x, event.y, acc);
                }
                tp->sync();
                auto after = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();
                runtimes.push_back(duration);

                output = acc.get();

                compute_stats();
                results.emplace_back(name, task_name, n_threads, buffer_size, events.size(), n_runs, mean, sd);

            }
        }
    }
};

void ThreadPoolBenchmark::cleanup() {}