#include <condition_variable>
#include <errno.h>
#include <functional>
#include <iostream>
#include <map>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>


int main() {
  auto n = 100;
  std::mutex* locks[n];
  auto threads = std::vector<std::thread>();
  for (int i = 0; i < n; i++) {
    std::mutex lock;
    locks[i] = &lock;
    lock.lock();
    threads.emplace_back([&, i] {
      {
        std::lock_guard<std::mutex> guard(*locks[i]);
        std::cout << "Thread " << i << " started" << std::endl;
      }
    });
  }

  auto before = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < n; i++) {
    locks[i]->unlock();
  }

  auto after = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(after - before)
          .count();
  std::cout << "Duration: " << duration << std::endl;
  
  for (int i = 0; i < n; i++) {
    threads[i].join();
  }

  return EXIT_SUCCESS;
}