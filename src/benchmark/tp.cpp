#include "benchmark/tp.hpp"
#include "task.hpp"
#include "threadpool.hpp"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

using namespace Async;

ThreadPoolBenchmark::ThreadPoolBenchmark(
    const std::string &name, const std::vector<size_t> buffer_sizes,
    const std::vector<size_t> thread_counts, const TaskType task,
    const std::string &task_name)
    : BaseBenchmark(name, buffer_sizes, thread_counts, task, task_name) {}

void ThreadPoolBenchmark::prepare(const size_t event_count) {

  auto acc = SimpleAccumulator();
  const int resolution = RAND_MAX / 1024;

  for (size_t i = 0; i < event_count; ++i) {
    const uint16_t x = std::rand() / resolution;
    const uint16_t y = std::rand() / resolution;
    task(x, y, acc);
    events.emplace_back(i, x, y, true);
  }

  checksum = acc.get();
}

CoroTask ThreadPoolBenchmark::run_task(const size_t x, const size_t y,
                                       AtomicAccumulator &acc) {
  //   scout() << "Before schedule: " << std::this_thread::get_id() << "\n";
  co_await tp->schedule();
  //   scout() << "After schedule: " << std::this_thread::get_id() << "\n";
  task(x, y, acc);
}

void ThreadPoolBenchmark::benchmark(const size_t n_runs,
                                    const std::vector<AER::Event> &events,
                                    const size_t checksum) {

  for (size_t buffer_size : buffer_sizes) {
    for (size_t thread_count : thread_counts) {

      for (size_t run = 0; run < n_runs; ++run) {
        // An atomic to hold the checksum
        AtomicAccumulator acc{};
        tp = std::make_unique<ThreadPool>(thread_count, buffer_size);

        auto before{std::chrono::high_resolution_clock::now()};
        for (const auto &event : events) {
          run_task(event.x, event.y, acc);
        }

        tp->sync();

        auto after{std::chrono::high_resolution_clock::now()};
        auto duration =
            std::chrono::duration_cast<std::chrono::nanoseconds>(after - before)
                .count();

        //   scout() << "Scheduled: " << tp->scheduled.load() << "\n";
        //   std::this_thread::sleep_for(1s);
        //   tp = nullptr;
        output = acc.get();
        if (output != checksum) {
          // Do something interesting.
          scout() << "WARNING: invalid checksum '" << output << "' (should be "
                  << checksum << ")\n";

          // throw std::runtime_error("Checksum failed");
        }

        runtimes.push_back(duration);
      }

      compute_stats();

      results.emplace_back(name, task_name, thread_count, buffer_size,
                           events.size(), n_runs, mean, sd);

      //   std::string fname{"results.csv"};
      //   scout() << "Events: " << event_count << " | buffer size: " <<
      //   buffer_size
      //           << " | threads: " << thread_count << "\n";

      //   std::ofstream out_file(fname, std::ios::app);
      //   for (const auto &r : results) {
      //     out_file << r.name << "," << r.events << "," << r.threads << ","
      //              << r.buffer_size << "," << r.n << "," << r.mean << "," <<
      //              r.std
      //              << "\n";
      //   }
      //   out_file.close();
    }
  }
};

void ThreadPoolBenchmark::cleanup() {}