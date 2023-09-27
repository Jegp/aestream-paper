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
    : BaseBenchmark(name, task, task_name), buffer_sizes(buffer_sizes),
      thread_counts(thread_counts) {}

CoroTask ThreadPoolBenchmark::run_task(const size_t x, const size_t y,
                                       Accumulator &acc) {
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

        tp->stop();

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
        tp.reset();
      }

      compute_stats();
      results.emplace_back(name, task_name, thread_count, buffer_size,
                           events.size(), n_runs, mean, sd);
    }
  }
};

void ThreadPoolBenchmark::cleanup() {
  tp->stop();
  tp->sync();
}
