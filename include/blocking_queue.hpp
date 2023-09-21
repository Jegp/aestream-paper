#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>

// Thanks to
// https://stackoverflow.com/questions/39512514/c-thread-safe-queue-shutdown
template <typename T> class queue {
private:
  std::mutex d_mutex;
  std::condition_variable d_condition;
  std::deque<T> d_queue;
  std::atomic<bool> _shutdown = false;

public:
  std::optional<T> pop() {

    std::unique_lock<std::mutex> lock(this->d_mutex);
    this->d_condition.wait(
        lock, [this] { return _shutdown.load() || !this->d_queue.empty(); });
    if (!_shutdown.load() || !this->d_queue.empty()) {
      T rc(std::move(this->d_queue.back()));
      this->d_queue.pop_back();
      return rc;
    } else {
      return std::nullopt;
    }
  }
  void push(T &value) {
    {
      std::unique_lock<std::mutex> lock(this->d_mutex);
      d_queue.emplace_back(std::move(value));
    }
    this->d_condition.notify_one();
  }
  void shutdown() {
    std::unique_lock<std::mutex> lock(this->d_mutex);
    _shutdown.store(true);
    this->d_condition.notify_all();
  }
};