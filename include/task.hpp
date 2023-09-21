#ifndef TASK_HPP
#define TASK_HPP
#include "stddef.h"
#include "stdint.h"
#include <atomic>
#include <thread>
#include <functional>



struct Accumulator
{
  virtual void add(const unsigned& x) = 0;
  size_t add_get(const unsigned& x)
  {
    add(x);
    return get();
  };
  virtual size_t get() = 0;
};

struct AtomicAccumulator: public Accumulator
{
  std::atomic<size_t> counter{ 0 };
  void add(const unsigned& x) { counter.fetch_add(x); }
  size_t add_get(unsigned x) { return counter.fetch_add(x); }
  size_t get() { return counter.load(); }
};

struct SimpleAccumulator: public Accumulator
{
  size_t counter{ 0 };
  void add(const unsigned& x) { counter += x; }
  virtual size_t get() { return counter; }
};

namespace Task
{
  using TaskType =
    std::function<void(const uint16_t& x, const uint16_t& y, Accumulator&)>;
  struct Simple
  {
    static void apply(const uint16_t& x, const uint16_t& y, Accumulator& T)
    {
      T.add_get(1);
    }
    inline static const std::string name = "Simple";
  };

  struct Buffer
  {
    static void apply(const uint16_t& x, const uint16_t& y, Accumulator& T)
    {
      auto current = T.add_get(1);
      if (current % 1000)
      {
        // Simulate 10us workload
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
    }
    inline static const std::string name = "Buffer";
  };

  struct Complex
  {
    static void apply(const uint16_t& x, const uint16_t& y, Accumulator& T)
    {
      size_t result{ 0 };

      for (int i = 0; i < x; ++i)
      {
        for (int j = 0; j < y; ++j)
        {
          result += i * i + j;
        }
      }

      T.add(result);
    }
    inline static const std::string name = "Complex";
  };
} // namespace Task


#endif // TASK_HPP
