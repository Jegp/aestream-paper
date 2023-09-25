#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include "aer.hpp"
#include "generator.hpp"
#include "noop.hpp"
#include "result.hpp"
#include "task.hpp"
#include "threadpool.hpp"

using namespace std::chrono_literals;
using namespace Async;
using namespace Task;

class BaseBenchmark {
public:
  BaseBenchmark(const std::string &name, const std::vector<size_t> event_counts,
                const std::vector<size_t> buffer_sizes,
                const std::vector<size_t> thread_counts, const TaskType task);

  virtual void benchmark(const size_t n_runs) = 0;
  void run(const size_t count);
  void compute_stats();

protected:
  std::string name{"Benchmark"};
  std::vector<size_t> event_counts;
  std::vector<size_t> buffer_sizes;
  std::vector<size_t> thread_counts;
  TaskType task;

  size_t checksum{0};
  double mean{0.0};
  double sd{0.0};
  size_t output{0};
  std::vector<size_t> runtimes;
  std::vector<AER::Event> events;
  std::vector<Result> results;
};

#endif // BENCHMARK_HPP
