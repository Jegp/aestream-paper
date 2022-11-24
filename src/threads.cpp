#include "threads.hpp"

void consumer(queue<std::vector<AEDAT::PolarityEvent>> *in,
              std::atomic_long *sum) {
  while (true) {
    std::optional<std::vector<AEDAT::PolarityEvent>> events = in->pop();
    if (events.has_value()) {
      for (auto event : events.value()) {
        sum->fetch_add(event.x + event.y);
      }
    } else {
      return;
    }
  }
}

void producer(std::vector<AEDAT::PolarityEvent> events,
              queue<std::vector<AEDAT::PolarityEvent>> *out, int buffer_size) {
  auto buffer = std::vector<AEDAT::PolarityEvent>();
  for (auto event : events) {
    buffer.push_back(event);
    if (buffer.size() >= buffer_size) {
      out->push(std::vector(buffer));
      buffer.clear();
    }
  }

  if (buffer.size() > 0) {
    out->push(buffer);
  }
  out->shutdown();
}

ThreadState prepare_threads(std::vector<AEDAT::PolarityEvent> events,
                            int buffer_size, int n_consumers) {
  const auto event_queue = new queue<std::vector<AEDAT::PolarityEvent>>();
  const auto sum_value = new std::atomic_long(0);
  auto consumers = new std::vector<std::thread *>();
  for (int i = 0; i < n_consumers; i++) {
    consumers->push_back(new std::thread(consumer, event_queue, sum_value));
  }
  return {buffer_size, consumers, events, event_queue, sum_value};
}

size_t run_threads(ThreadState state) {
  auto p_thread =
      std::thread(producer, state.events, state.event_queue, state.buffer_size);

  p_thread.join();

  std::thread **threads = state.consumer_threads->data();
  for (int i = 0; i < state.consumer_threads->size(); i++) {
    threads[i]->join();
  }
  return state.sum_value->load();
}