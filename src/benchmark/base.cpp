#include <cmath>
#include <numeric>
#include <chrono>
#include <cmath>
#include <ctime>

#include "benchmark/base.hpp"

BaseBenchmark::BaseBenchmark(const std::string& name,
                             const std::vector<size_t> event_counts,
                             const std::vector<size_t> buffer_sizes,
                             const std::vector<size_t> thread_counts,
                             const TaskType task)
    : name(name), event_counts(event_counts),
    buffer_sizes(buffer_sizes), thread_counts(thread_counts),
    task(task)
{

}


void BaseBenchmark::run(const size_t n_runs)
{

    try
    {
        // Run the benchmark
        this->benchmark(n_runs);

        if (checksum != output)
        {
            std::cerr << checksum << " != " << output << std::endl;
            throw std::runtime_error("Checksum failed");
        }
    }
    catch (std::exception& e)
    {
        std::cout << "Stream ending for " << name
            << ": " << e.what()
            << std::endl;
    }

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
