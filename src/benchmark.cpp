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

#include "task.hpp"
#include "threadpool.hpp"
// #include "threads.hpp"

using namespace std::chrono_literals;
using namespace Async;
using TaskType =
    std::function<void(const uint16_t &x, const uint16_t &y, Accumulator &)>;

//
// Coroutines
//
Generator<AER::Event> event_generator(const std::vector<AER::Event> &events) {
  for (const auto &event : events) {
    co_yield event;
  }
}

template <class Task> size_t coroutine_sum(Generator<AER::Event> events) {
  size_t count = 0;
  for (const auto &event : events) {
    count += Task::apply(event.x, event.y);
  }
  return count;
}

// Parallel coroutines
ReturnObject<AER::Event> bench_co_par(const std::vector<AER::Event> &events, TaskType task,
                                      const size_t &checksum,
                                      std::vector<size_t> &times,
                                      const size_t n_threads) {
  std::atomic<bool> done{false};
  std::mutex awaiter_list_lock;
  std::vector<std::jthread> threads{};
  std::vector<ThreadPromise<AER::Event> *> awaiters = {};
  for (size_t i = 0; i < n_threads; i++) {
    threads.push_back(std::jthread([&]() mutable {
      thread_runner<AER::Event>(awaiters, awaiter_list_lock, done);
    }));
  }
  while (awaiters.size() < n_threads) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Run benchmark
  // std::atomic<long> sum_co_par{0};
  // auto sum_co_lambda = [&](AER::Event e) {
  // sum_co_par.fetch_add(Task::Complex::apply(e.x, e.y)); };
  // auto sum_co_lambda = [&](AER::Event e) {
  // sum_co_par.fetch_add(Task::Complex::apply(e.x, e.y));
  // };
  auto acc = AtomicAccumulator();
  auto sum_co_lambda = [&](AER::Event e) { task(e.x, e.y, acc); };
  ReturnPromise<AER::Event> *object =
      co_await GetPromise<ReturnPromise<AER::Event>>{};
  object->awaiters = awaiters;
  object->set_callback(sum_co_lambda);

  // std::cout << "Awaiters " << object->awaiters.size() << std::endl;

  // Run
  auto before = std::chrono::high_resolution_clock::now();
  for (auto event : events) {
    co_yield event;
  }
  auto after = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(after - before)
          .count();

  std::this_thread::sleep_for(100ms);

  // Reset
  if (acc.get() != checksum) {
    // std::cout << "Duration: " << duration << std::endl;
    std::cout << "Checksum failed: " << acc.get() << " != " << checksum
              << std::endl;
  }
  times.push_back(duration);

  cleanup_coroutines<AER::Event>(threads, awaiters, done);
}

//
// Benchmarking function
//
std::tuple<float, float> bench_fun(
    const std::string &name, std::function<size_t()> f, const size_t check,
    int n, std::function<void()> prepare_f = []() { return; }) {
  auto times = std::vector<size_t>();

  for (int i = 0; i < n; i++) {
    prepare_f();

    auto before = std::chrono::high_resolution_clock::now();
    try {
      size_t out = f();

      if (check != out) {
        std::cerr << check << " != " << out << std::endl;
        throw std::runtime_error("Checksum failed");
      }
    } catch (std::exception &e) {
      std::cout << "Stream ending for " << name << ": " << e.what()
                << std::endl;
    }
    auto after = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(after - before)
            .count();
    times.push_back(duration);
  }
  auto mean = accumulate(times.begin(), times.end(), 0.0) / times.size();
  float dev = 0.0;
  for (auto time : times) {
    dev += pow(time - mean, 2);
  }
  auto stddev = sqrt(dev / times.size());
  return {mean, stddev};
}

std::vector<Result> run_once(size_t n_events, size_t n_runs,
                             std::vector<size_t> buffer_sizes, const std::string name, TaskType task) {
  auto events = std::vector<AER::Event>();
  auto results = std::vector<Result>();
  auto acc_simple = SimpleAccumulator();
  auto acc_buffer = SimpleAccumulator();
  auto acc_complex = SimpleAccumulator();
  const int resolution = RAND_MAX / 1024;
  TaskType tasks = {Task::Simple(), Task::Complex(), Task::Buffer()};

  for (size_t i = 0; i < n_events; ++i) {
    const uint16_t x = std::rand() / resolution;
    const uint16_t y = std::rand() / resolution;
    Task::Simple::apply(x, y, acc_simple);
    Task::Buffer::apply(x, y, acc_buffer);
    Task::Complex::apply(x, y, acc_complex);
    events.emplace_back(i, x, y, true);
  }
  const size_t check_simple = acc_simple.get();
  const size_t check_buffer = acc_buffer.get();
  const size_t check_complex = acc_complex.get();

  // Single thread
  auto single_run = [&](const std::string &name, TaskType task,
                        const size_t checksum) {
    return bench_fun(
        name,
        [&] {
          auto acc = SimpleAccumulator();
          for (const auto &event : events) {
            task(event.x, event.y, acc);
          }
          return acc.get();
        },
        checksum, n_runs);
  };
  auto [sm, ss] = single_run("single_" + name, task, check_simple);
  results.push_back({"single_" + task.name, 0, 0, n_events, n_runs, sm, ss});

  // ThreadPool benchmark
  std::vector<size_t> threads = {1, 2, 4, 8};
  auto run_threadpool = [&](auto task, const std::string &name,
                            const size_t checksum, size_t n_threads,
                            size_t buffer_size) {
    std::unique_ptr<ThreadPool> tp{nullptr};
    auto [mean, std] = bench_fun(
        name,
        [&]() {
          for (const auto &event : events) {
            tp->enqueue(task, event.x, event.y);
          }
          tp.sync();
          return tp->total_sum.load();
        },
        checksum, n_runs,
        [&]() { tp = std::make_unique<ThreadPool>(n_threads, buffer_size); });

    Result res =
        Result{name, n_threads, buffer_size, n_events, n_runs, mean, std};
    results.push_back(res);
  };
  for (auto task : tasks) {
    for (size_t buffer_size : buffer_sizes) {
      for (size_t n_threads : threads) {
        run_threadpool(task, "tp_simple", check_simple,
                       n_threads, buffer_size);
        // run_threadpool(Task::Complex::apply, "tp_complex", check_complex,
        //  n_threads, buffer_size);
      }
    }
  }

  // Parallel coroutine benchmark
  auto threads = std::thread::hardware_concurrency();
  if (threads > 2) {
    threads -= 1;
  }
  for (auto task : tasks) {
    std::vector<size_t> times{};
    for (int i = 0; i < n_runs; i++) {
      bench_co_par(events, task.apply, check_buffer, times, threads);
    }
    auto mean = accumulate(times.begin(), times.end(), 0.0) / times.size();
    float dev = 0.0;
    for (auto time : times) {
      dev += pow(time - mean, 2);
    }
    auto stddev = sqrt(dev / times.size());

    results.emplace_back("conoop_" + task.name, 0, 0, events.size(), n_runs,
                         mean, stddev);
  }

  for (auto task : tasks) {
    ThreadPoolBenchmark tpb{"ThreadPool", buffer_sizes,
                            thread_counts, Task::Simple{}.apply};

    tpb.run(n_runs);
  }

  // Return all results
  return results;
}

int main(int argc, char const *argv[]) {
  std::srand(std::time(nullptr));
  // std::filesystem::remove("results.csv");

  int N = 4;
  // std::vector<size_t> buffer_sizes = {512, 1024, 2048, 4096, 8192, 16384};
  std::vector<size_t> buffer_sizes = {512, 4096, 16384};

  for (int i = 50; i < 70; i++) {
    // for (int i = 109; i < 114; i++) {
    auto n_events = long(pow(1.2, i));
    // auto n_events = 100000;
    std::cout << "Running " << n_events << " repeated " << N << " times"
              << std::endl;
    auto results = run_once(n_events, N, buffer_sizes);

    std::string fname{"results.csv"};
    std::ofstream out_file(fname, std::ios::app);
    for (auto r : results) {
      out_file << r.name << "," << r.events << "," << r.threads << ","
               << r.buffer_size << "," << r.n << "," << r.mean << "," << r.std
               << "\n";
    }
    out_file.close();
  }
}
