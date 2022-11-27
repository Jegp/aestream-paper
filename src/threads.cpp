#include "threads.hpp"

void consumer(queue<EventPtr> *in, std::atomic_long *sum) {
  while (true) {
    auto opt = in->pop();
    if (opt.has_value()) {
      for (auto event : *opt.value()) {
        sum->fetch_add(event.x + event.y);
      }
    } else {
      return;
    }
  }
}

EventPtr reserve_buffer(size_t buffer_size) {
  auto buffer = EventPtr{new EventVec()};
  buffer->reserve(buffer_size);
  return buffer;
}

void producer(const EventVec &events, queue<EventPtr> *out,
              size_t buffer_size) {
  EventPtr buffer = reserve_buffer(buffer_size);
  for (auto event : events) {
    buffer->push_back(event);
    if (buffer->size() >= buffer_size) {
      out->push(buffer);
      buffer = reserve_buffer(buffer_size);
    }
  }

  if (buffer->size() > 0) {
    out->push(buffer);
  }
  out->shutdown();
}

ThreadState prepare_threads(const EventVec &events, const size_t buffer_size,
                            size_t n_consumers) {
  auto event_queue = new queue<EventPtr>();
  auto consumers = new EventThreads();
  auto sum_value = new std::atomic_long(0);
  for (size_t i = 0; i < n_consumers; i++) {
    auto t = new std::thread(consumer, event_queue, sum_value);
    consumers->push_back(t);
  }
  return {buffer_size, event_queue, std::move(consumers), events, sum_value};
}

size_t run_threads(const ThreadState &state) {
  auto p_thread = std::thread(producer, std::cref(state.events),
                              state.event_queue, state.buffer_size);

  p_thread.join();

  auto threads = state.consumer_threads->data();
  for (size_t i = 0; i < state.consumer_threads->size(); i++) {
    threads[i]->join();
  }
  return state.sum_value->load();
}