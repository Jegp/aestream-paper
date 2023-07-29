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

#include "aer.hpp"
#include "generator.hpp"

#include "task.hpp"
#include "threadpool.hpp"
#include "threads.hpp"

using namespace std::chrono_literals;
using namespace Async;
using TaskType = std::function<size_t(uint16_t x, uint16_t y)>;

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

//
// Benchmarking function
//
std::tuple<float, float> bench_fun(
    const std::string &name, std::function<size_t()> f, const size_t check,
    int n, std::function<void()> prepare_f = [] {}) {
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

struct Result {
  std::string name;
  size_t threads;
  size_t buffer_size;
  size_t events;
  size_t n;
  float mean;
  float std;
};

std::vector<Result> run_once(size_t n_events, size_t n_runs,
                             std::vector<size_t> buffer_sizes) {
  auto events = std::vector<AER::Event>();
  auto results = std::vector<Result>();
  size_t check_simple = 0;
  size_t check_complex = 0;
  const int resolution = RAND_MAX / 1024;

  for (size_t i = 0; i < n_events; i++) {
    const uint16_t x = std::rand() / resolution;
    const uint16_t y = std::rand() / resolution;
    check_simple += Task::Simple::apply(x, y);
    check_complex += Task::Complex::apply(x, y);
    events.emplace_back(i, x, y, true);
  }

  // Single thread
  auto single_run = [&](const std::string &name, TaskType task,
                        size_t checksum) {
    return bench_fun(
        name,
        [&] {
          size_t count = 0;
          for (const auto &event : events) {
            count += task(event.x, event.y);
          }
          return count;
        },
        checksum, n_runs);
  };
  auto [sm1, ss1] =
      single_run("single_simple", Task::Simple::apply, check_simple);
  results.push_back({"single_simple", 0, 0, n_events, n_runs, sm1, ss1});
  auto [sm2, ss2] =
      single_run("single_complex", Task::Complex::apply, check_complex);
  results.push_back({"single_complex", 0, 0, n_events, n_runs, sm2, ss2});

  // Coroutines
  auto run_coroutine = [&](auto task, const size_t check,
                           const std::string &name) {
    auto [mean, std] = bench_fun(
        name,
        [&]() {
          return coroutine_sum<decltype(task)>(event_generator(events));
        },
        check, n_runs);
    results.push_back({name, 0, 0, n_events, n_runs, mean, std});
  };

  run_coroutine(Task::Simple{}, check_simple, "c_simple");
  run_coroutine(Task::Complex{}, check_complex, "c_complex");

  // Threads
  // auto run_threads = [&](auto task, size_t check, const std::string &name,
  //                        size_t t, size_t buffer_size) {
  //   using TS = ThreadState<decltype(task)>;
  //   TS ts{events, buffer_size, t};
  //   auto [mean, std] = bench_fun(
  //       [&] {
  //         unsigned long l = ts.run();
  //         std::cout << l << std::endl;
  //         return l;
  //       },
  //       check, n_runs);
  //   results.emplace_back(name, t, buffer_size, n_events, n_runs, mean, std);
  // };

  // for (size_t buffer_size : buffer_sizes) {
  //   for (size_t t : threads) {
  //     run_threads(Task::Simple{}, check_simple, "t_simple", t, buffer_size);
  //   }
  // }

  // Threadpool benchmark
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

          tp->sync();

          return tp->total_sum.load();
        },
        checksum, n_runs,
        [&] { tp = std::make_unique<ThreadPool>(n_threads, buffer_size); });
    Result res =
        Result{name, n_threads, buffer_size, n_events, n_runs, mean, std};
    results.push_back(res);
  };

  for (size_t buffer_size : buffer_sizes) {
    for (size_t n_threads : threads) {
      run_threadpool(Task::Simple::apply, "tp_simple", check_simple, n_threads,
                     buffer_size);
      run_threadpool(Task::Complex::apply, "tp_complex", check_complex,
                     n_threads, buffer_size);
    }
  }

  return results;
}

int main(int argc, char const *argv[]) {
  std::srand(std::time(nullptr));
  // std::filesystem::remove("results.csv");

  int N = 128;
  std::vector<size_t> buffer_sizes = {512, 1024, 2048, 4096, 8192, 16384};

  // for (int i = 10; i < 11; i++)
  // for (int i = 50; i < 60; i++)
  for (int i = 109; i < 114; i++) {
    auto n_events = long(pow(1.2, i));
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
