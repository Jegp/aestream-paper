#include <chrono>
#include <cmath>
#include <ctime>
#include <numeric>
#include <syncstream>

#include "benchmark/base.hpp"

static auto scout = []() { return std::osyncstream(std::cout); };

BaseBenchmark::BaseBenchmark(
    const std::string& name,
    const TaskType task,
    const std::string& task_name)
    : name(name), task(task), task_name(task_name)
{
}

void BaseBenchmark::run(
    const size_t runs,
    const std::vector<AER::Event>& events,
    const size_t checksum)
{
    try
    {
        for (size_t buffer_size : buffer_sizes)
        {
            for (size_t thread_count : thread_counts)
            {
                for (size_t run = 0; run < runs; ++run)
                {
                    // Run the benchmark
                    auto [output, duration] = this->benchmark(
                        run,
                        runs,
                        thread_count,
                        thread_counts,
                        buffer_size,
                        buffer_sizes,
                        events,
                        checksum);
                    //   scout() << "Scheduled: " << tp->scheduled.load() << "\n";
                    //   std::this_thread::sleep_for(1s);
                    //   tp = nullptr;
                    if (output != checksum)
                    {
                        // Do something interesting.
                        scout() << "WARNING: invalid checksum '" << output << "' (should be "
                                << checksum << ")\n";

                        // throw std::runtime_error("Checksum failed");
                    }

                    runtimes.push_back(duration);
                }

                compute_stats();
                //            scout() << "Mean: " << mean << ", SD: " << sd << "\n";

                result.emplace_back(
                    name, task_name, thread_count, buffer_size, events.size(), runs, mean, sd);
            }
        }
    }
    catch (std::exception& e)
    {
        std::cout << "Stream ending for benchmark '" << name << "': " << e.what() << std::endl;
    }
}

void BaseBenchmark::update(std::vector<Result>& results)
{
    results.insert(results.end(), result.begin(), result.end());
}

void BaseBenchmark::compute_stats()
{
    mean = std::accumulate(runtimes.begin(), runtimes.end(), 0.0) / runtimes.size();

    float dev = 0.0;
    for (auto runtime : runtimes)
    {
        dev += pow(runtime - mean, 2);
    }

    sd = std::sqrt(dev / (runtimes.size() - 1));
}
