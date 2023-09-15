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

auto conditions = std::map<unsigned, std::function<void()>>{};
static void empty_handler(int signum, siginfo_t *info, void *context) {
  auto id = std::hash<std::thread::id>{}(std::this_thread::get_id());
  conditions[id](); 
}

/* Install empty handler for the specified signal.
 * Returns 0 if success, errno error code otherwise.
 */
static int install_empty_handler(const int signum) {
  struct sigaction act {};

  memset(&act, 0, sizeof act);
  sigemptyset(&act.sa_mask);
  act.sa_sigaction = empty_handler;
  act.sa_flags = SA_SIGINFO;
  if (sigaction(signum, &act, NULL) == -1)
    return errno;

  return 0;
}

int main() {
  int ret = install_empty_handler(SIGINT);
  if (ret != 0) {
    fprintf(stderr, "install_empty_handler() failed: %s\n", strerror(ret));
    return EXIT_FAILURE;
  }

  std::mutex* locks[n];
  auto threads = std::vector<std::thread>();
  for (int i = 0; i < n; i++) {
    std::binary_semaphore lock{0};
    locks[i] = &lock;
    threads.emplace_back([&] {
      unsigned id = std::hash<std::thread::id>{}(std::this_thread::get_id());
      conditions.emplace(id, [] {});
      lock.acquire();
    });
  }

  auto before = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < n; i++) {
    pthread_kill(threads[i].native_handle(), SIGINT);
  }
  auto after = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(after - before)
          .count();
  std::cout << "Duration: " << duration << std::endl;

  for (int i = 0; i < n; i++) {
    locks[i]->unlock();
  }

  printf("Press Ctrl+C to exit\n");
  while (1) {
    sleep(1);
  }

  return EXIT_SUCCESS;
}