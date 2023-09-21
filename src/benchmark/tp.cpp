#include "benchmark/tp.hpp"
#include "threadpool.hpp"
#include <cstddef>
#include <fstream>

using namespace Async;

ThreadPoolBenchmark::ThreadPoolBenchmark(const std::string& name,
                                         const std::vector<size_t> event_sizes,
                                         const std::vector<size_t> buffer_sizes,
                                         const std::vector<size_t> thread_counts,
                                         const TaskType task)
    :
    BaseBenchmark(name, event_sizes, buffer_sizes, thread_counts, task)
{}

void ThreadPoolBenchmark::prepare(const size_t n_threads,
                                  const size_t n_buffer_size,
                                  const size_t event_count)
{



    auto acc = SimpleAccumulator();
    const int resolution = RAND_MAX / 1024;

    for (size_t i = 0; i < event_count; ++i)
    {
        const uint16_t x = std::rand() / resolution;
        const uint16_t y = std::rand() / resolution;
        task(x, y, acc);
        events.emplace_back(i, x, y, true);
    }

    checksum = acc.get();
    tp = std::make_unique<ThreadPool>();
}

void ThreadPoolBenchmark::benchmark(const size_t n_runs)
{

    for (size_t event_count : event_counts)
    {
        for (size_t buffer_size : buffer_sizes)
        {
            for (size_t n_threads : thread_counts)
            {
                prepare(n_threads, buffer_size, event_count);

                for (size_t i = 0; i < n_runs; ++i)
                {
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
                    results.emplace_back(name, n_threads, buffer_size, event_count, n_runs, mean, sd);


                    std::string fname{ "_results.csv" };
                    std::ofstream out_file(fname, std::ios::app);
                    for (auto r : results)
                    {
                        out_file << r.name << "," << r.events << "," << r.threads
                            << "," << r.buffer_size << ","
                            << r.n << "," << r.mean << "," << r.std << "\n";
                    }
                    out_file.close();
                }
            }
        }
    }
};

void ThreadPoolBenchmark::cleanup() {}