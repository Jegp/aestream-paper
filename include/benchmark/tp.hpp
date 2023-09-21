#ifndef THREADPOOLBENCHMARK_HPP
#define THREADPOOLBENCHMARK_HPP

#include "benchmark/base.hpp"
#include <vector>

class ThreadPoolBenchmark: public BaseBenchmark
{
public:
  ThreadPoolBenchmark(const std::string& name,
                      const std::vector<size_t> event_sizes,
                      const std::vector<size_t> buffer_sizes,
                      const std::vector<size_t> thread_counts,
                      const TaskType task);


  void prepare(const size_t n_threads, const size_t n_buffer_size, const size_t event_count);
  void cleanup();
  virtual void benchmark(const size_t n_runs);

private:
  uptr<ThreadPool> tp{ nullptr };
  size_t current_tp_size;
  std::vector<size_t> buffer_sizes{};
  std::vector<size_t> thread_counts{};
};

#endif // THREADPOOLBENCHMARK_HPP
