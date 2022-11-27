#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "aedat.hpp"
#include "blocking_queue.cpp"

using EventVec = std::vector<AEDAT::PolarityEvent>;
using EventThreads = std::vector<std::thread *>;
using EventPtr = std::unique_ptr<EventVec>;

struct ThreadState {
  const size_t buffer_size;
  queue<EventPtr> *event_queue;
  const EventThreads *consumer_threads;
  const EventVec &events;
  std::atomic_long *sum_value;
};

void consumer(queue<EventPtr> *in, std::atomic_long *sum);

void producer(const EventVec &events, queue<EventPtr> *out, size_t buffer_size);

ThreadState prepare_threads(const EventVec &events, size_t buffer_size,
                            size_t n_consumers);

size_t run_threads(const ThreadState &state);