#ifndef TASK_HPP
#define TASK_HPP
#include "stddef.h"
#include "stdint.h"
#include <atomic>
#include <functional>
#include <thread>

struct Accumulator {
  virtual void add(const size_t x) = 0;
  virtual size_t add_get(const size_t x) {
    this->add(x);
    return this->get();
  };
  virtual size_t get() = 0;
};

struct AtomicAccumulator : public Accumulator {
  std::atomic<size_t> counter{0};
  virtual void add(const size_t x) override { counter.fetch_add(x); }
  virtual size_t add_get(size_t x) override { return counter.fetch_add(x); }
  virtual size_t get() override { return counter.load(); }
};

struct SimpleAccumulator : public Accumulator {
  size_t counter{0};
  virtual void add(const size_t x) override { counter += x; }
  virtual size_t get() override { return counter; }
};

namespace Task
{

  using TaskType =
    std::function<void(const uint16_t x, const uint16_t y, Accumulator*)>;

  struct Simple
  {
    static void apply(const uint16_t x, const uint16_t y, Accumulator* T)
    {
        T->add(1);
    }
    inline static const std::string name = "Simple";
  };

struct Buffer {
  static void apply(const uint16_t x, const uint16_t y, Accumulator* T) {
    if (T->add_get(1) % 1000 == 0) {
        // Simulate 10us workload
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }
  inline static const std::string name = "Buffer";
};

struct Complex {
    static void apply(const uint16_t x, const uint16_t y, Accumulator* T) {
    size_t result{0};

    for (size_t i = 0; i < x; ++i) {
      for (size_t j = 0; j < y; ++j) {
        result += i * i + j;
      }
    }

    T->add(result);
  }
  inline static const std::string name = "Complex";
};
} // namespace Task

#endif // TASK_HPP
