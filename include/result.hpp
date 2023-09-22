#pragma once

#include <string>

struct Result
{
  std::string name;
  std::string task;
  size_t threads;
  size_t buffer_size;
  size_t events;
  size_t n;
  double mean;
  double std;
};
