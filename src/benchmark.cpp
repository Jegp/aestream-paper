#include <cassert>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>

#include "aer.hpp"
#include "generator.hpp"
#include "noop.hpp"
#include "result.hpp"

#include "benchmark/coro_threads.hpp"
#include "benchmark/threadless.hpp"
#include "benchmark/tp.hpp"
#include "task.hpp"

// Parallel coroutines
// ReturnObject<AER::Event> bench_co_par(
//     const std::vector<AER::Event>& events,
//     TaskType task,
//     const size_t& checksum,
//     std::vector<size_t>& times,
//     const size_t n_threads)
// {
//     std::atomic<bool> done{false};
//     std::mutex awaiter_list_lock;
//     std::vector<std::jthread> threads{};
//     std::vector<ThreadPromise<AER::Event>*> awaiters = {};
//     for (size_t i = 0; i < n_threads; i++)
//     {
//         threads.push_back(std::jthread(
//             [&]() mutable { thread_runner<AER::Event>(awaiters, awaiter_list_lock, done); }));
//     }
//     while (awaiters.size() < n_threads)
//     {
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//     }

//     // Run benchmark
//     auto acc                          = AtomicAccumulator();
//     auto sum_co_lambda                = [&](AER::Event e) { task(e.x, e.y, acc); };
//     ReturnPromise<AER::Event>* object = co_await GetPromise<ReturnPromise<AER::Event>>{};
//     object->awaiters                  = awaiters;
//     object->set_callback(sum_co_lambda);

//     // std::cout << "Awaiters " << object->awaiters.size() << std::endl;

//     // Run
//     auto before = std::chrono::high_resolution_clock::now();
//     for (auto event : events)
//     {
//         co_yield event;
//     }
//     std::this_thread::sleep_for(std::chrono::nanoseconds(events.size() / 1000));
//     auto after    = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();

//     // Reset
//     if (acc.get() != checksum)
//     {
//         // std::cout << "Duration: " << duration << std::endl;
//         std::cout << "Checksum failed: " << acc.get() << " != " << checksum << std::endl;
//     }
//     times.push_back(duration);

//     cleanup_coroutines<AER::Event>(threads, awaiters, done);
// }

// //
// // Benchmarking function
// //
// std::tuple<float, float> bench_fun(
//     std::function<size_t()> f,
//     const size_t check,
//     int n,
//     std::function<void()> prepare_f = []() { return; })
// {
//     auto times = std::vector<size_t>();

//     for (int i = 0; i < n; i++)
//     {
//         prepare_f();

//         auto before = std::chrono::high_resolution_clock::now();
//         try
//         {
//             size_t out = f();

//             if (check != out)
//             {
//                 std::cerr << check << " != " << out << std::endl;
//                 throw std::runtime_error("Checksum failed");
//             }
//         }
//         catch (std::exception& e)
//         {
//             std::cout << "Stream ending: " << e.what() << std::endl;
//         }
//         auto after = std::chrono::high_resolution_clock::now();
//         auto duration =
//             std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();
//         times.push_back(duration);
//     }
//     auto mean = accumulate(times.begin(), times.end(), 0.0) / times.size();
//     float dev = 0.0;
//     for (auto time : times)
//     {
//         dev += pow(time - mean, 2);
//     }
//     auto stddev = sqrt(dev / times.size());
//     return {mean, stddev};
// }

std::tuple<std::vector<AER::Event>, size_t> generate_events(size_t n_events, TaskType task)
{
    auto events          = std::vector<AER::Event>();
    auto acc_simple      = SimpleAccumulator();
    const int resolution = RAND_MAX / 1024;

    for (size_t i = 0; i < n_events; ++i)
    {
        const uint16_t x = std::rand() / resolution;
        const uint16_t y = std::rand() / resolution;
        task(x, y, acc_simple);
        events.emplace_back(i, x, y, true);
    }
    const size_t check_simple = acc_simple.get();
    return {events, check_simple};
}

int main(int argc, char const* argv[])
{
    std::srand(std::time(nullptr));
    std::vector<size_t> buffer_sizes  = {1024};
    std::vector<size_t> thread_counts = {16, 24, 32};
    auto results                      = std::vector<Result>{};
    size_t n_runs                     = 8;
    auto threads                      = std::thread::hardware_concurrency();
    if (threads > 2)
    {
        threads -= 1;
    }

    // for (int i = 109; i < 114; i++) {
    for (int i = 50; i < 55; i++)
    {
        auto event_count                     = long(pow(1.2, i));
        auto [events_simple, check_simple]   = generate_events(event_count, Task::Simple::apply);
        auto [events_buffer, check_buffer]   = generate_events(event_count, Task::Buffer::apply);
        auto [events_complex, check_complex] = generate_events(event_count, Task::Complex::apply);

        std::cout << "Running " << event_count << " repeated " << n_runs << " times" << std::endl;

        // Threadless (single thread)
        ////////////////////////////////////////
        // Simple task
        ThreadlessBenchmark tlb_simple{
            "Single-thread benchmark | Simple task",
            buffer_sizes,
            {1},
            Task::Simple::apply,
            Task::Simple::name,
        };
        tlb_simple.run(n_runs, events_simple, check_simple);
        tlb_simple.update(results);

        // Buffered task
        ThreadlessBenchmark tlb_buffer{
            "Single-thread benchmark | Buffered task",
            buffer_sizes,
            {1},
            Task::Buffer::apply,
            Task::Buffer::name,
        };
        tlb_buffer.run(n_runs, events_simple, check_simple);
        tlb_buffer.update(results);

        // Buffered task
        ThreadlessBenchmark tlb_complex{
            "Single-thread benchmark | Complex task",
            buffer_sizes,
            {1},
            Task::Complex::apply,
            Task::Complex::name,
        };
        tlb_complex.run(n_runs, events_simple, check_simple);
        tlb_complex.update(results);

        // Mutexed threadpool
        ////////////////////////////////////////
        // Simple task
        ThreadPoolBenchmark tpb_simple{
            "Mutexed ThreadPool benchmark | Simple task",
            buffer_sizes,
            thread_counts,
            Task::Simple::apply,
            Task::Simple::name,
        };
        tpb_simple.run(n_runs, events_simple, check_simple);
        tpb_simple.update(results);

        // Buffered task
        ThreadPoolBenchmark tpb_buffer{
            "Mutexed ThreadPool benchmark | Buffered task",
            buffer_sizes,
            thread_counts,
            Task::Buffer::apply,
            Task::Buffer::name,
        };
        tpb_buffer.run(n_runs, events_buffer, check_buffer);
        tpb_buffer.update(results);

        // Complex task
        ThreadPoolBenchmark tpb_complex{
            "Mutexed ThreadPool benchmark | Complex task",
            buffer_sizes,
            thread_counts,
            Task::Complex::apply,
            Task::Complex::name,
        };
        tpb_complex.run(n_runs, events_complex, check_complex);
        tpb_complex.update(results);

        // Coroutines with threads
        ////////////////////////////////////////
        // Simple task
        CoroThreadBenchmark ctb_simple{
            "(Lossy) coroutine benchmark with threads | Simple task",
            buffer_sizes,
            thread_counts,
            Task::Simple::apply,
            Task::Simple::name,
        };
        ctb_simple.run(n_runs, events_simple, check_simple);
        ctb_simple.update(results);

        // Buffered task
        CoroThreadBenchmark ctb_buffer{
            "(Lossy) coroutine benchmark with threads | Buffered task",
            buffer_sizes,
            thread_counts,
            Task::Buffer::apply,
            Task::Buffer::name,
        };
        ctb_buffer.run(n_runs, events_buffer, check_buffer);
        ctb_buffer.update(results);

        // Complex task
        CoroThreadBenchmark ctb_complex{
            "(Lossy) coroutine benchmark with threads | Complex task",
            buffer_sizes,
            thread_counts,
            Task::Complex::apply,
            Task::Complex::name,
        };
        ctb_complex.run(n_runs, events_complex, check_complex);
        ctb_complex.update(results);

        // Write the results to a file
        ////////////////////////////////////////
        std::cout << "Writing to disk" << std::endl;
        std::string fname{"results.csv"};
        std::ofstream out_file(fname, std::ios::app);
        for (auto r : results)
        {
            out_file << r.name << "," << r.task << "," << r.events << "," << r.threads << ","
                     << r.buffer_size << "," << r.n << "," << r.mean << "," << r.std << "\n";
        }
        out_file.close();
    }
}
