#include <cmath>
#include <numeric>
#include <chrono>
#include <cmath>
#include <ctime>

#include "benchmark/base.hpp"

BaseBenchmark::BaseBenchmark(const std::string& name,
                             const TaskType task, const std::string task_name, const size_t checksum)
    : name(name), 
    task(task), checksum(checksum), task_name(task_name)
{
}


void BaseBenchmark::run(const size_t n_runs, const std::vector<AER::Event>& events)
{

    try
    {
        // Run the benchmark
        this->benchmark(n_runs, events);

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
