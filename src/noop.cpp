#include <chrono>
#include <coroutine>

#include "noop.hpp"

using ReturnPromise = ReturnObject::promise_type;
using ThreadPromise = ThreadAwaiter<ReturnPromise>::promise_type;
using AwaiterList = std::vector<ThreadPromise *>;

std::atomic<int> current_awaiter{0};

ThreadAwaiter<ReturnPromise> do_dumb_stuff(
    std::coroutine_handle<ReturnPromise> h,
    ThreadAwaiter<ReturnObject::promise_type>::promise_type *awaiter) {
  awaiter->return_handle = {h};
  co_await *awaiter;
}

std::coroutine_handle<> ReturnPromise::await_suspend(
    std::coroutine_handle<ReturnObject::promise_type> h) {
  int index = current_awaiter++ % awaiters.size();
  auto awaiter = awaiters[index];
  do_dumb_stuff(h, awaiter);
  return std::noop_coroutine();
}

std::atomic<bool> done{false};
std::mutex awaiter_list_lock;

ReturnObject producer(AwaiterList &awaiter_promises) {
  std::cout << "producer " << std::this_thread::get_id() << std::endl;
  ReturnObject::promise_type *object = co_await GetPromise<ReturnPromise>{};
  object->awaiters = awaiter_promises;
  object->callback = [](int value) {
    std::cout << "callback " << value << " from thread "
              << std::this_thread::get_id() << std::endl;
  };
  for (int i = 0; i < 20; i++) {
    object->value = i;
    co_await *object;
  }
  done = true;
  co_return;
}

ThreadAwaiter<ReturnObject::promise_type> thread_runner(AwaiterList &awaiters) {
  std::cout << "thread " << std::this_thread::get_id() << std::endl;
  auto promise = co_await GetPromise<
      ThreadAwaiter<ReturnObject::promise_type>::promise_type>{};
  {
    std::lock_guard<std::mutex> lock(awaiter_list_lock);
    awaiters.push_back(promise);
  }
  while (!done) {
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (promise->return_handle.has_value()) {
      promise->return_handle.value().resume();
      promise->return_handle.reset();
    }
  }
}

int main() {

  auto n_threads = 20;
  std::cout << "main " << std::this_thread::get_id() << std::endl;
  AwaiterList awaiters = {};
  std::vector<std::jthread> threads;
  for (int i = 0; i < n_threads; i++) {
    threads.push_back(std::jthread([&]() mutable { thread_runner(awaiters); }));
  }

  while (awaiters.size() < n_threads) {
    std::cout << "waiting for threads" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  ReturnObject coro = producer(awaiters);

  for (auto &thread : threads) {
    thread.join();
  }
}