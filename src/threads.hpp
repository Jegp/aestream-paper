#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "aedat.hpp"
#include "blocking_queue.cpp"

using EventVec = std::vector<AEDAT::PolarityEvent>;
using EventThreads = std::vector<std::thread>;
using EventPtr = std::unique_ptr<EventVec>;

template<class Task>
class ThreadState {
  const size_t buffer_size;
  queue<EventPtr> event_queue;
  EventThreads consumer_threads;
  const EventVec &events;
  std::atomic_long sum_value;

  void consumer();
  static EventPtr reserve_buffer(size_t buffer_size);
  void producer();

public:
  ThreadState(const EventVec &ev, size_t buf_size,
              size_t n_consumers);

  size_t run();
};

#include "threads.cpp"
