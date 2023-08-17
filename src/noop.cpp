#include <chrono>
#include <coroutine>

#include "noop.hpp"


// template <typename T>
// ReturnObject copar_producer(
// std::vector<ThreadAwaiter<ReturnObject::promise_type>::promise_type *>
// awaiter_promises,
// std::vector<T> &data, std::function<void(T)> callback,
// std::atomic<bool> &done) {
// ReturnObject::promise_type *object = co_await GetPromise<ReturnPromise>{};
// object->awaiters = awaiter_promises;
// object->callback = callback;
// std::cout << "Awaiters " << object->awaiters.size() << std::endl;
// int i = 0;
// try {
// for (; i < data.size(); i++) {
// std::cout << "producer " << std::this_thread::get_id() << " " << data[i]
// << " " << object << std::endl;
// object->value = data[i];
// co_await *object;
// }
// } catch (std::exception &e) {
// std::cout << "producer " << std::this_thread::get_id() << " " << e.what()
// << std::endl;
// }
// std::cout << "Ran until " << i << std::endl;
// done.store(true);
// co_return;
// }

// ThreadAwaiter<ReturnObject::promise_type>
// thread_runner(AwaiterList &awaiters, std::mutex &awaiter_list_lock,
// std::atomic<bool> &done) {
// }

// std::unique_ptr<NoopState> prepare_coroutines(size_t n_threads) {
//   std::unique_ptr<NoopState> ptr = std::make_unique<NoopState>();
//   for (int i = 0; i < n_threads; i++) {
//     ptr->threads.push_back(
//         std::jthread([&]() mutable { thread_runner(ptr->awaiters); }));
//   }
//   std::cout << "Created threads: " << ptr->threads.size() << std::endl;
//   while (ptr->awaiters.size() < n_threads) {
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//   }
//   std::cout << "Created awaiters: " << ptr->awaiters.size() << std::endl;
//
//   return ptr;
// }

// template <typename T>
// void run_coroutines(
// std::vector<ThreadAwaiter<ReturnObject::promise_type>::promise_type *>
// awaiters,
// std::vector<T> &data, std::function<void(T)> task,
// std::atomic<bool> &done) {
// producer<T>(awaiters, data, task, done);
// }

// template <typename T>
// unsigned int ReturnObject<T>::current_awaiter = 0;


// int main2() {
//
//   auto n_threads = 10;
//   std::cout << "main " << std::this_thread::get_id() << std::endl;
//   AwaiterList awaiters = {};
//   std::vector<std::jthread> threads;
//   for (int i = 0; i < n_threads; i++) {
//     threads.push_back(std::jthread([&]() mutable { thread_runner(awaiters);
//     }));
//   }
//
//   while (awaiters.size() < n_threads) {
//     std::cout << "waiting for threads" << std::endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//   }
//
//   std::vector<int> data = {1, 2, 3, 4, 5};
//   std::function<void(int)> callback = [](int value) {
//     std::cout << "callback " << std::this_thread::get_id() << " " << value
//               << std::endl;
//   };
//   ReturnObject coro = producer(awaiters, data, callback);
//
//   for (auto &thread : threads) {
//     thread.join();
//   }
// }
