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
  BaseBenchmark(const std::string &name, const TaskType task,
                const std::string &task_name);

  virtual void benchmark(const size_t n_runs,
                         const std::vector<AER::Event> &events,
                         const size_t checksum) = 0;
  void run(const size_t n_runs, const std::vector<AER::Event> &events,
           const size_t checksum);
  void compute_stats();

  std::vector<Result> results;

protected:
  std::string name{"Benchmark"};
  TaskType task;
  const std::string task_name;

  double mean{0.0};
  double sd{0.0};
  size_t output{0};
  std::vector<size_t> runtimes;
  std::vector<AER::Event> events;
};

#endif // BENCHMARK_HPP
