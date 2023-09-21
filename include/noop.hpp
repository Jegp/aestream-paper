#pragma once

#include <coroutine>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>

#include "result.hpp"

template <typename PromiseType> struct GetPromise
{
  PromiseType* obj;
  bool await_ready() { return false; } // False: call await_suspend
  bool await_suspend(std::coroutine_handle<PromiseType> h)
  {
    obj = &h.promise();
    return false; // False: don't suspend
  }
  PromiseType* await_resume() { return obj; }
};

template <typename PromiseType, std::movable T> struct ThreadAwaiter
{
  struct promise_type
  {
    std::exception_ptr current_exception;
    ThreadAwaiter<PromiseType, T> get_return_object()
    {
      return ThreadAwaiter<PromiseType, T>{
        .coro = std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::optional<std::function<void(T)>> callback{};
    std::optional<T> value;
    std::coroutine_handle<PromiseType> return_handle;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
    {
      if (current_exception)
      {
        std::rethrow_exception(current_exception);
      }
      return std::noop_coroutine();
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() noexcept
    {
      current_exception = std::current_exception();
    }
    bool await_ready() { return false; }
    void await_resume() {}
  };
  std::coroutine_handle<promise_type> coro;
  operator std::coroutine_handle<>() { return coro; }
};

template <std::movable T> struct ReturnObject
{
  static inline unsigned int current_awaiter{ 0 };

  struct promise_type
  {
    using ReturnType = ReturnObject<T>;
    using ThreadAwaiterType =
      ThreadAwaiter<ReturnType::promise_type, T>::promise_type;
    std::vector<ThreadAwaiterType*> awaiters;
    std::exception_ptr current_exception;
    ReturnObject<T> get_return_object()
    {
      return ReturnObject{
          .coro = std::coroutine_handle<promise_type>::from_promise(*this) };
    }
    // std::coroutine_handle<>
    // await_suspend(std::coroutine_handle<ReturnObject::promise_type> h) {
    // int index = current_awaiter++ % awaiters.size();
    // auto awaiter = awaiters[index];
    // awaiter->value = value;
    // do_dumb_stuff<T>(h, awaiter);
    // return std::noop_coroutine();
    // }
    std::suspend_never yield_value(T value)
    {
      int index = current_awaiter++ % awaiters.size();
      // auto coro = std::coroutine_handle<promise_type>::from_promise(*this);
      int attempt = 0;
      while (awaiters[index]->value.has_value())
      {
        std::this_thread::yield();
        index = current_awaiter++ % awaiters.size();
        attempt++;
        if (attempt >= awaiters.size())
        {
          break;
        }
      }

      // awaiter->return_handle = coro;
      // while (awaiter->value.has_value()) {
      // std::this_thread::yield();
      // }
      awaiters[index]->value.emplace(std::move(value));
      // awaiter->callback_lock.unlock();
      // do_dumb_stuff<T>(coro, awaiter);
      return std::suspend_never{};
    }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() { current_exception = std::current_exception(); }
    bool await_ready() { return false; }
    void await_resume()
    {
      if (current_exception)
      {
        std::rethrow_exception(current_exception);
      }
    }
    void set_callback(std::function<void(T)> callback)
    {
      for (auto& awaiter : awaiters)
      {
        awaiter->callback = callback;
      }
    }
  };
  std::coroutine_handle<promise_type> coro;
  operator std::coroutine_handle<>() { return coro; }
  explicit operator bool() const { return !coro.done(); }
  // T operator()() { return coro.promise().value; }
  // ~ReturnObject() {
  // if (coro) {
  // coro.destroy();
  // }
  // }
};
// template <typename T> unsigned int ReturnObject<T>::current_awaiter = 0;

// struct NoopState {
// std::vector<ThreadAwaiter<ReturnObject::promise_type>::promise_type *>
// awaiters = {};
// std::vector<std::jthread> threads = {};
// };

template <typename T> using ReturnPromise = ReturnObject<T>::promise_type;
template <std::movable T>
using ThreadPromise = ThreadAwaiter<ReturnPromise<T>, T>::promise_type;
template <std::movable T> using AwaiterList = std::vector<ThreadPromise<T>*>;

template <std::movable T>
ThreadAwaiter<ReturnPromise<T>, T>
thread_runner(std::vector<ThreadPromise<T>*>& awaiters,
              std::mutex& awaiter_list_lock, std::atomic<bool>& done)
{
  auto promise = co_await GetPromise<ThreadPromise<T>>{};
  {
    std::lock_guard<std::mutex> lock(awaiter_list_lock);
    awaiters.push_back(promise);
  }
  // promise->callback_lock.lock(); // Lock until woken up
  while (!done)
  {
    {
      if (promise->value.has_value())
      {
        promise->callback.value()(promise->value.value());
        promise->value.reset();
        // promise->return_handle.resume();
        // promise->value.reset();
        // } else {
      }
      // std::this_thread::yield();
      // promise->callback_lock.try_lock_for(du);
      // std::lock_guard<std::mutex> lock(promise->callback_lock);
      // promise->callback_lock.unlock();

      // promise->return_handle.resume();
    }
  }
  // promise->get_return_object().coro.destroy();
  // if (promise->return_handle.has_value()) {
  // promise->return_handle.value().resume();
  // promise->return_handle.value().destroy();
  // promise->get_return_object().coro.destroy();
  // }
}

template <typename T, std::movable V>
ThreadAwaiter<ReturnPromise<T>, V>
do_dumb_stuff(std::coroutine_handle<ReturnPromise<T>> h,
              ThreadPromise<T>* awaiter)
{
  // if (awaiter->return_handle.has_value()) {
  // !awaiter->return_handle.value().done()) {
  if (!awaiter->return_handle.has_value())
  {
    // awaiter->return_handle = {h};
    co_await *awaiter;
  }
  else
  {
    std::cout << "Has value" << std::endl;
  }
}
// template <typename T>
// ReturnObject<T>
// copar_producer(std::vector<ThreadPromise<T> *> awaiters, std::vector<T>
// &data,
//  std::function<void(T)> callback, std::atomic<bool> &done) {
// ReturnPromise<T> *object = co_await GetPromise<ReturnPromise<T>>{};
// object->awaiters = awaiters;
// object->callback = callback;
// int i = 0;
// for (; i < data.size(); i++) {
// object->value = data[i];
// co_await *object;
// }
// done.store(true);
// }

template <std::movable T>
void cleanup_coroutines(std::vector<std::jthread>& threads,
                        std::vector<ThreadPromise<T>*>& awaiters,
                        std::atomic<bool>& done)
{
  // std::cout << "Cleaning up" << std::endl;
  done.store(true);
  for (auto& thread : threads)
  {
    thread.join();
  }
}