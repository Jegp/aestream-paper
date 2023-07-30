#pragma once

#include <coroutine>
#include <iostream>
#include <thread>

template <typename PromiseType> struct GetPromise {
  PromiseType *obj;
  bool await_ready() { return false; }
  bool await_suspend(std::coroutine_handle<PromiseType> h) {
    obj = &h.promise();
    return false;
  }
  PromiseType *await_resume() { return obj; }
};

template <typename PromiseType> struct ThreadAwaiter {
  struct promise_type {
    ThreadAwaiter<PromiseType> get_return_object() {
      return ThreadAwaiter<PromiseType>{
          .coro = std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::coroutine_handle<PromiseType> return_handle;
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) {
      return std::noop_coroutine();
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    bool await_ready() {
      // std::cout << "await_ready " << value << std::endl;
      return false;
    }
    void await_resume() {}
  };
  std::coroutine_handle<promise_type> coro;
  operator std::coroutine_handle<>() { return coro; }
};

struct ReturnObject {

  struct promise_type {
    ThreadAwaiter<ReturnObject::promise_type>::promise_type *awaiter;
    int value{0};
    ReturnObject get_return_object() {
      return ReturnObject{
          .coro = std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<promise_type> h);
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    bool await_ready() {
      std::cout << "await_ready " << value << std::endl;
      return false;
    }
    void await_resume() { std::cout << "await_resume " << value << std::endl; }
  };
  std::coroutine_handle<promise_type> coro;
  operator std::coroutine_handle<>() { return coro; }
};
