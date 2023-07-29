#include <cassert>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <functional>
#include <numeric>
#include <sstream>
#include <string>
#include <fstream>

#include "aer.hpp"
#include "generator.hpp"

#include "task.hpp"
#include "threads.hpp"
#include "threadpool.hpp"

using namespace std::chrono_literals;
using namespace Async;

//
// Coroutines
//
Generator<AER::Event>
event_generator(const std::vector<AER::Event>& events)
{
  for (const auto& event : events)
  {
    co_yield event;
  }
}

template <class Task>
size_t coroutine_sum(Generator<AER::Event> events)
{
  size_t count = 0;
  for (const auto& event : events)
  {
    count += Task::apply(event.x, event.y);
  }
  return count;
}

size_t single_sum(const std::vector<AER::Event>& events)
{
  size_t count = 0;
  for (const auto& event : events)
  {
    count += event.x + event.y;
  }
  return count;
}

//
// Benchmarking function
//
template <typename T>
std::tuple<float, float> bench_fun(std::function<size_t(T&)> f,
  std::function<T& (void)> prepare_f,
  size_t check, int n)
{
  auto times = std::vector<size_t>();

  for (int i = 0; i < n; i++)
  {
    auto& prepared = prepare_f();
    auto before = std::chrono::high_resolution_clock::now();
    try
    {

      size_t out = f(prepared);

      if (check != out)
      {
        std::cerr << check << " != " << out << std::endl;
        throw std::runtime_error("");
      }

    }
    catch (std::runtime_error& e)
    {
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
  for (auto time : times)
  {
    dev += pow(time - mean, 2);
  }
  auto stddev = sqrt(dev / times.size());
  return { mean, stddev };
}

struct Result
{
  std::string name;
  size_t threads;
  size_t buffer_size;
  size_t events;
  size_t n;
  float mean;
  float std;
};

std::vector<Result> run_once(size_t n_events, size_t n_runs,
  std::vector<size_t> buffer_sizes)
{
  auto events = std::vector<AER::Event>();
  auto results = std::vector<Result>();
  size_t check_simple = 0;
  size_t check_complex = 0;
  const int resolution = RAND_MAX / 1024;


  std::cout << "Resolution: " << resolution << "\n";

  for (size_t i = 0; i < n_events; i++)
  {
    const uint16_t x = std::rand() / resolution;
    const uint16_t y = std::rand() / resolution;
    check_simple += Task::Simple::apply(x, y);
    check_complex += Task::Complex::apply(x, y);
    // auto event = AER::Event{ i, x, y, true, true };
    events.emplace_back(i, x, y, true);
  }

  // throw std::runtime_error("");
  // Single thread
  auto [sm1, ss1] = bench_fun<std::vector<AER::Event>>(
    single_sum, [&] { return std::ref<std::vector<AER::Event>>(events); }, check_simple, n_runs);
  results.push_back({ "single_simple", 0, 0, n_events, n_runs, sm1, ss1 });

  // Coroutines
  auto run_coroutine = [&](auto task, size_t check, const std::string& name)
    {
      auto [mean, std] = bench_fun<std::vector<AER::Event>>(
        [&](std::vector<AER::Event>& events)
        {
          return coroutine_sum<decltype(task)>(event_generator(events));
        },
        [&] { return std::ref<std::vector<AER::Event>>(events); }, check, n_runs);
      results.push_back({ name, 0, 0, n_events, n_runs, mean, std });
    };

  run_coroutine(Task::Simple{}, check_simple, "c_simple");
  run_coroutine(Task::Complex{}, check_complex, "c_complex");

  // auto [cm2, cs2] = bench_fun<std::vector<AER::Event>>(
  //     [&](std::vector<AER::Event> &events) {
  //       return coroutine_sum_slow(event_generator(events));
  //     },
  //     [&events] { return events; }, check_complex, n_runs);
  // results.push_back({"c_slow", 0, 0, n_events, n_runs, cm2, cm2});

  // Threads

  auto run_threads = [&](auto task, size_t check, const std::string& name,
    size_t t, size_t buffer_size)
    {
      using TS = ThreadState<decltype(task)>;
      TS ts{ events, buffer_size, t };
      auto [mean, std] = bench_fun<TS>([](TS& t) { return t.run(); },
        [&]
        {
          return std::ref<TS>(ts);
        },
        check, n_runs);

      results.emplace_back(name, t, buffer_size, n_events, n_runs, mean, std );
    };

  std::vector<size_t> threads = { 1, 2, 4, 8 };
  for (size_t buffer_size : buffer_sizes)
  {
    for (size_t t : threads)
    {
      run_threads(Task::Simple{}, check_simple, "t_simple", t, buffer_size);
    }
  }
  // for (size_t buffer_size : buffer_sizes) {
  //   for (size_t t : threads) {
  //     auto [mean2, std2] = bench_fun<ThreadState>(
  //         [](ThreadState &t) { return t.run(); },
  //         [&] {
  //           return ThreadState{"complex", events, buffer_size, t};
  //         },
  //         check_complex, n_runs);
  //     results.push_back(
  //         {"t_complex", t, buffer_size, n_events, n_runs, mean2, std2});
  //   }
  // }

  // Threadpool benchmark
  auto run_threadpool = [&](
    auto task,
    const std::string& name,
    size_t n_threads,
    size_t buffer_size
    )
    {
      ThreadPool tp{ n_threads, buffer_size };
      auto [mean, std] = bench_fun<ThreadPool>([&](ThreadPool& tp)
        {
          for (const auto& event : events)
          {
            tp.enqueue(task, event.x, event.y);
          }

          tp.sync();

          log("Final sum: ", tp.total_sum.load());
          return tp.total_sum.load();
        },
        [&]
        {
          tp.wait();
          return std::ref<ThreadPool>(tp);
        },
        check_simple, n_runs);
      Result res = Result{ name, n_threads, buffer_size, n_events, n_runs, mean, std };
      results.push_back(res);
    };


  for (size_t buffer_size : buffer_sizes)
  {
    for (size_t n_threads : threads)
    {
      run_threadpool(Task::Simple::apply, "tp_simple", n_threads, buffer_size);
    }
  }

  return results;
}

int main(int argc, char const* argv[])
{
  std::srand(std::time(nullptr));
  // std::filesystem::remove("results.csv");

  int N = 3;
  std::vector<size_t> buffer_sizes = { 512, 1024, 2048, 4096, 8192, 16384 };

  for (int i = 10; i < 32; i++)
    // for (int i = 109; i < 114; i++)
  {
    auto n_events = long(pow(1.2, i));
    std::cout << "Running " << n_events << " repeated " << N << " times"
      << std::endl;
    auto results = run_once(n_events, N, buffer_sizes);

    std::string fname{ "results.csv" };
    std::ofstream out_file(fname, std::ios::app);
    for (auto r : results)
    {
      out_file << r.name << "," << r.events << "," << r.threads << ","
        << r.buffer_size << "," << r.n << "," << r.mean << "," << r.std
        << "\n";
    }
    out_file.close();
  }
}
