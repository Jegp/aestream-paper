#include <coroutine>

#include "noop.hpp"

ThreadAwaiter<ReturnObject::promise_type> do_dumb_stuff(
    std::coroutine_handle<ReturnObject::promise_type> h,
    ThreadAwaiter<ReturnObject::promise_type>::promise_type *awaiter) {
  awaiter->return_handle = h;
  co_await *awaiter;
}

std::coroutine_handle<> ReturnObject::promise_type::await_suspend(
    std::coroutine_handle<ReturnObject::promise_type> h) {
  std::cout << "await_suspend " << h.promise().value << std::endl;
  // Get awaiter
  // Set value in awaiter
  // awaiter->return_handle = h;
  // Await awaiter
  // co_await *awaiter;
  do_dumb_stuff(h, awaiter);

  // std::jthread t([h]() mutable {
  //   std::cout << "thread " << std::this_thread::get_id() << " "
  //             << h.promise().value << std::endl;
  //   h.resume();
  // });
  // return std::coroutine_handle<promise_type>::from_promise(*this);
  return std::noop_coroutine();
}

ReturnObject producer() {
  ReturnObject::promise_type *object =
      co_await GetPromise<ReturnObject::promise_type>{};
  for (int i = 0; i < 20; i++) {
    object->value = i;
    co_await *object;
  }
}

ThreadAwaiter<ReturnObject::promise_type> thread_runner() {
  std::cout << "thread " << std::this_thread::get_id() << std::endl;
  auto promise = co_await GetPromise<
      ThreadAwaiter<ReturnObject::promise_type>::promise_type>{};
  while (true) {
    std::cout << "Looping" << std::endl;
    if (promise->return_handle != nullptr) {
      std::cout << "thread " << std::this_thread::get_id() << " "
                << promise->return_handle.promise().value << std::endl;
      promise->return_handle.resume();
    }
    promise->return_handle = nullptr;
  }
}

int main() {
  auto awaiter_thread = std::jthread(thread_runner);

  std::coroutine_handle<> coro = producer();
  // for (int i = 0; i < 10; i++) {
  // std::cout << "main " << std::this_thread::get_id() << std::endl;
  // coro.resume();
  // }
  coro.destroy();
}