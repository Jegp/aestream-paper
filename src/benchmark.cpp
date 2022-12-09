#include <iostream>
#include <vector>
#include <numeric>

#include <cppcoro/task.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/generator.hpp>
#include <cppcoro/when_all.hpp>

#include "threads.hpp"
#include "task.hpp"

using cppcoro::generator;

//
// Coroutines
//
generator<Event>
event_generator(const std::vector<Event> &events) {
  for (auto event : events) {
    co_yield event;
  }
}

template<class Task>
cppcoro::task<size_t> coroutine_sum(generator<Event> events) {
  size_t count = 0;
  for (const auto &event : events) {
    count += Task::apply(event.x, event.y);
  }
  co_return count;
}

template<class Task>
cppcoro::task<size_t> coroutine_sum_shared_generator(generator<Event> &events) {
  size_t count = 0;
  for (const auto &event : events) {
    count += Task::apply(event.x, event.y);
  }
  co_return count;
}

cppcoro::task<size_t> coroutine_aggregate_results(std::vector<cppcoro::task<size_t>> &&tasks) {
    std::vector<size_t> results = co_await when_all(std::move(tasks));
    size_t result{};
    for (auto r: results) result += r;
    co_return result;
}


//
// Benchmarking function
//
template <typename T>
std::tuple<float, float> bench_fun(std::function<size_t(T &)> f,
                                   std::function<T(void)> prepare_f,
                                   size_t check, int n) {
  auto times = std::vector<size_t>();

  for (int i = 0; i < n; i++) {
    auto prepared = prepare_f();
    auto before = std::chrono::high_resolution_clock::now();
    try {
      size_t out = f(prepared);
      if (check != out) {
        std::cerr << check << " != " << out;
        throw std::runtime_error("");
      }

    } catch (std::runtime_error &e) {
      std::cout << "Stream ending: " << e.what() << std::endl;
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
  auto events = std::vector<Event>();
  auto results = std::vector<Result>();
  size_t check_simple = 0;
  size_t check_complex = 0;
  const int resolution = RAND_MAX / 1024;

  for (size_t i = 0; i < n_events; i++) {
    const uint16_t x = std::rand() / resolution;
    const uint16_t y = std::rand() / resolution;
    check_simple += Task::Simple::apply(x, y);
    check_complex += Task::Complex::apply(x, y);
    auto event = Event{x, y};
    events.push_back(event);
  }

  // Coroutines, single producer single consumer
  auto run_coroutine = [&](auto task, size_t check, const std::string &name) {
    auto [mean, std] = bench_fun<std::vector<Event>>(
        [&](std::vector<Event> &events) {
          return cppcoro::sync_wait(coroutine_sum<decltype(task)>(event_generator(events)));
        },
        [&events] { return events; }, check, n_runs);
    results.push_back({name, 0, 0, n_events, n_runs, mean, std});
  };

  run_coroutine(Task::Simple{}, check_simple, "c_simple");
  run_coroutine(Task::Complex{}, check_complex, "c_complex");

  // Coroutines, single producer multiple consumers
  auto run_coroutine_single_producer_multiple_consumers = [&](auto task, size_t check, const std::string &name, size_t t) {
    auto [mean, std] = bench_fun<std::vector<Event>>(
        [&](std::vector<Event> &events) {
            std::vector<cppcoro::task<size_t>> coroutine_tasks;

            cppcoro::generator<Event> generator{event_generator(events)};

            for (int i = 0; i < t; ++i) {
                coroutine_tasks.emplace_back(coroutine_sum_shared_generator<decltype(task)>(generator));
            }

            auto coroutine_task = coroutine_aggregate_results(std::move(coroutine_tasks));
            return cppcoro::sync_wait(coroutine_task);
        },
        [&events] { return events; }, check, n_runs);
    results.push_back({name, 0, 0, n_events, n_runs, mean, std});
  };

  // This call crashes
  // run_coroutine_single_producer_multiple_consumers(Task::Simple{}, check_simple, "c_single_producer_multiple_consumers_simple", 2);

  // Coroutines, multiple producers multiple consumers
  auto run_coroutine_multiple_producer_multiple_consumers = [&](auto task, size_t check, const std::string &name, size_t t) {
    auto [mean, std] = bench_fun<std::vector<Event>>(
        [&](std::vector<Event> &events) {
            std::vector<cppcoro::task<size_t>> coroutine_tasks;

            for (int i = 0; i < t; ++i) {
                coroutine_tasks.emplace_back(coroutine_sum<decltype(task)>(std::move(event_generator(events))));
            }

            auto coroutine_task = coroutine_aggregate_results(std::move(coroutine_tasks));
            return cppcoro::sync_wait(coroutine_task);
        },
        [&events] { return events; }, check, n_runs);
    results.push_back({name, 0, 0, n_events, n_runs, mean, std});
  };

  run_coroutine_multiple_producer_multiple_consumers(Task::Simple{}, check_simple, "c_multiple_producers_multiple_consumers_simple", 2);

  // Threads

  auto run_threads = [&](auto task, size_t check, const std::string &name, size_t t, size_t buffer_size) {
      using TS = ThreadState<decltype(task)>;
      auto [mean, std] = bench_fun<TS>(
          [](TS &t) { return t.run(); },
          [&] {
            return TS{events, buffer_size, t};
          },
          check, n_runs);
      results.push_back(
          {name, t, buffer_size, n_events, n_runs, mean, std});
  };

  std::vector<size_t> threads = {1, 2, 4, 8};
  for (size_t buffer_size : buffer_sizes) {
    for (size_t t : threads) {
      run_threads(Task::Simple{}, check_simple, "t_simple", t, buffer_size);
      run_threads(Task::Complex{}, check_complex, "t_complex", t, buffer_size);
    }
  }

  return results;
}

int main(int argc, char const *argv[]) {
  std::srand(std::time(nullptr));
  // std::filesystem::remove("results.csv");

  int N = 1;
  std::vector<size_t> buffer_sizes = {4096};

  // for (int i = 10; i < 32; i++) {
  for (int i = 65; i < 66; i++) {
    auto n_events = long(pow(1.2, i));
    std::cout << "Running " << n_events << " repeated " << N << " times"
              << std::endl;
    auto results = run_once(n_events, N, buffer_sizes);

    // std::ofstream out_file("results.csv", std::ios_base::app);
    for (auto r : results) {
      std::cout << r.name << "," << r.events << "," << r.threads << ","
               << r.buffer_size << "," << r.n << "," << r.mean << "," << r.std
               << "\n";
    }
    // out_file.close();
  }
}
