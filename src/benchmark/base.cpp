#include <chrono>
#include <cmath>
#include <ctime>
#include <numeric>

#include "benchmark/base.hpp"

BaseBenchmark::BaseBenchmark( const std::string& name, const TaskType task, const std::string& task_name )
    : name( name ), task( task ), task_name( task_name )
{
}

void BaseBenchmark::run( const size_t n_runs,
                         const std::vector<AER::Event>& events,
                         const size_t checksum )
{
    try
    {
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
    mean =
        std::accumulate( runtimes.begin(), runtimes.end(), 0.0 ) / runtimes.size();

    float dev = 0.0;
    for ( auto runtime : runtimes )
    {
        dev += pow( runtime - mean, 2 );
    }

    sd = std::sqrt( dev / ( runtimes.size() - 1 ) );
}
