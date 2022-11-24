#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "aedat.hpp"
#include "blocking_queue.cpp"

struct ThreadState {
  int buffer_size;
  std::vector<std::thread *> *consumer_threads;
  std::vector<AEDAT::PolarityEvent> &events;
  queue<std::vector<AEDAT::PolarityEvent>> *event_queue;
  std::atomic_long *sum_value;
};

void consumer(queue<std::vector<AEDAT::PolarityEvent>> *in,
              std::atomic_long *sum);

void producer(std::vector<AEDAT::PolarityEvent> &events,
              queue<std::vector<AEDAT::PolarityEvent>> *out, int buffer_size);

ThreadState prepare_threads(std::vector<AEDAT::PolarityEvent> &events,
                            int buffer_size, int n_consumers);

size_t run_threads(ThreadState state);