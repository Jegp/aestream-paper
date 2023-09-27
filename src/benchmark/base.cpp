#include <chrono>
#include <cmath>
#include <ctime>
#include <numeric>

#include "benchmark/base.hpp"
#include "benchmark/tp.hpp"

BaseBenchmark::BaseBenchmark( const std::string& name,
                              const std::vector<size_t> buffer_sizes,
                              const std::vector<size_t> thread_counts,
                              const TaskType task,
                              const std::string& task_name )
    : name( name ), buffer_sizes( buffer_sizes ), thread_counts( thread_counts ), task( task ), task_name( task_name )
{
}

void BaseBenchmark::run( const size_t n_runs,
                         const std::vector<AER::Event>& events,
                         const size_t checksum )
{
    try
    {
        std::cout << "Running benchmark '" << name << "'\n";

        // Run the benchmark
        this->benchmark( n_runs, events, checksum );
    }
    catch ( std::exception& e )
    {
        std::cout << "Stream ending for " << name << ": " << e.what() << std::endl;
    }
}

void BaseBenchmark::compute_stats()
{
    mean = std::accumulate( runtimes.begin(), runtimes.end(), 0.0 ) / runtimes.size();

    float dev = 0.0;
    for ( auto runtime : runtimes )
    {
        dev += pow( runtime - mean, 2 );
    }

    std::cout << "Runtimes: " << runtimes.size() << "\n";
    std::sqrt( dev / ( runtimes.size() - 1 ) );
}
