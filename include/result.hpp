#pragma once

#include <string>

struct Result
{
  std::string name;
  size_t threads;
  size_t buffer_size;
  size_t events;
  size_t n;
  double mean;
  double std;
};
