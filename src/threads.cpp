#include "threads.hpp"

void ThreadState::consumer() {
  while (true) {
    auto opt = event_queue.pop();
    if (opt.has_value()) {
      size_t local_sum = 0;
      for (const auto &event : *opt.value()) {
            local_sum += event.x + event.y;
      }
      sum_value.fetch_add(local_sum);
    } else {
      return;
    }
  }
}

EventPtr ThreadState::reserve_buffer(size_t buffer_size) {
  auto buffer = EventPtr{new EventVec};
  buffer->reserve(buffer_size);
  return buffer;
}

void ThreadState::producer() {
  EventPtr buffer = reserve_buffer(buffer_size);
  for (const auto &event : events) {
    buffer->push_back(event);
    if (buffer->size() >= buffer_size) {
      event_queue.push(buffer);
      buffer = reserve_buffer(buffer_size);
    }
  }

  if (buffer->size() > 0) {
    event_queue.push(buffer);
  }
  event_queue.shutdown();
}

ThreadState::ThreadState(const EventVec &ev, size_t buf_size, size_t n_consumers) :
buffer_size{buf_size},
events{ev},
sum_value{0}
{
  for (size_t i = 0; i < n_consumers; i++) {
    consumer_threads.emplace_back(std::thread{&ThreadState::consumer, this});
  }
}

size_t ThreadState::run() {
  auto p_thread = std::thread(&ThreadState::producer, this);

  p_thread.join();

  for (auto &thread: consumer_threads) {
    thread.join();
  }
  return sum_value.load();
}