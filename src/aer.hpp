#pragma once

#include <memory>

struct AER {
  struct Event {
    uint64_t timestamp;
    uint16_t x;
    uint16_t y;
    bool polarity;

    friend std::ostream &operator<<(std::ostream &os, const Event &event) {
      os << "Event(" << event.timestamp << ", " << event.x << ", " << event.y
         << ", " << event.polarity << ")";
      return os;
    }
  };
};
