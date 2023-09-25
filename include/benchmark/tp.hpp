#ifndef THREADPOOLBENCHMARK_HPP
#define THREADPOOLBENCHMARK_HPP

#include "benchmark/base.hpp"
#include <vector>

class ThreadPoolBenchmark : public BaseBenchmark {
public:
  ThreadPoolBenchmark(const std::string &name,
                      const std::vector<size_t> buffer_sizes,
                      const std::vector<size_t> thread_counts,
                      const TaskType task, const std::string &task_name);

  void prepare(const size_t n_threads, const size_t n_buffer_size);
  void cleanup();
  CoroTask run_task(const size_t x, const size_t y, AtomicAccumulator &acc);
  virtual void benchmark(const size_t n_runs,
                         const std::vector<AER::Event> &events,
                         const size_t checksum);

private:
  uptr<ThreadPool> tp{nullptr};
  size_t current_tp_size;
  std::vector<size_t> buffer_sizes{};
  std::vector<size_t> thread_counts{};
};

#endif // THREADPOOLBENCHMARK_HPP
