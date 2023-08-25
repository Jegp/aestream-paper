#pragma once

#include <coroutine>
#include <functional>
#include <optional>
#include <thread>

namespace coroutinestd = std; // TODO: add support for clang

template <typename PromiseType> struct GetPromise {
  PromiseType *obj;
  bool await_ready() { return false; } // False: call await_suspend
  bool await_suspend(std::coroutine_handle<PromiseType> h) {
    obj = &h.promise();
    return false; // False: don't suspend
  }
  PromiseType *await_resume() { return obj; }
};

template <typename T, typename V> struct ThreadAwaiter {
  struct promise_type {
    std::exception_ptr current_exception;
    ThreadAwaiter<T> get_return_object() {
      return ThreadAwaiter<T>{
          .coro = std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::optional<V> value{};
  }
}

template <std::movable T>
class Generator {
  // Thanks to https://en.cppreference.com/w/cpp/coroutine/coroutine_handle
public:
  struct promise_type {
    std::optional<T> current_value;
    std::exception_ptr current_exception;

    Generator<T> get_return_object() {
      return Generator{Handle::from_promise(*this)};
    }
    void return_void() {}
    // use coroutinestd instead of {std, std::experimental}
    static coroutinestd::suspend_always initial_suspend() noexcept {
      return {};
    }
    static coroutinestd::suspend_always final_suspend() noexcept { return {}; }
    coroutinestd::suspend_never yield_value(T value) noexcept {
      current_value = std::move(value);
      return {};
    }
    // Disallow co_await in generator coroutines.
    void await_transform() = delete;
    void unhandled_exception() noexcept {
      current_exception = std::current_exception();
    }
  };

  // use coroutinestd instead of {std, std::experimental}
  using Handle = coroutinestd::coroutine_handle<promise_type>;
  struct HandleState {
    std::atomic<bool> done{false};
    std::vector<const Handle> handles = {};
    size_t current_index{0};
    const Handle current_handle() { return handles[current_index]; }
    const Handle next_handle() {
      size_t index = current_index++ % handles.size();
      return handles[index];
    }
  };
  using HandleState = std::unique_ptr<HandleState>;

  explicit Generator(const HandleState state) : state{state} {}

  Generator() = default;
  ~Generator() {
    for (const auto &handle : state->handles) {
      if (handle) {
        handle.destroy();
      }
    }
  }

  Generator(const Generator &) = delete;
  Generator &operator=(const Generator &) = delete;

  Generator(Generator &&other) noexcept : m_coroutine{other.m_coroutine} {
    other.m_coroutine = {};
  }
  Generator &operator=(Generator &&other) noexcept {
    if (this != &other) {
      if (m_coroutine) {
        m_coroutine.destroy();
      }
      m_coroutine = other.m_coroutine;
      other.m_coroutine = {};
    }
    return *this;
  }

  // Range-based for loop support.
  class Iter {
  public:
    void operator++() { state->next_handle().resume(); }
    const T &operator*() const {
      promise_type promise = state->current_handle().promise();
      if (promise.current_exception) {
        std::rethrow_exception(promise.current_exception);
      } else {
        return *promise.current_value;
      }
    }
    bool operator==(std::default_sentinel_t) const {
      // return !m_coroutine || m_coroutine.done();
      return state->done.load();
    }

    explicit Iter(const HandleState state) : state{state} {}

  private:
    HandleState state;
  };

  Iter begin() {
    if (state->handles.size() == 0) {
      state->handles[0].resume();
    }
    return Iter{state};
  }
  std::default_sentinel_t end() { return {}; }

private:
  HandleState state;
};