#pragma once

#include "stddef.h"
#include "stdint.h"

struct Event {
    // 12 bytes in total
    uint16_t x, y;
    uint16_t extra[10];
};


namespace Task {
    struct Simple {
        static size_t apply(uint16_t x, uint16_t y) {
            return x + y;
        }
    };

    struct Complex {
        static size_t apply(uint16_t x, uint16_t y) {
            auto c = new char[1000];
            for (int i = 0; i < 1000; ++i) {
                c[i] = x;
            }
            size_t r{};
            for (int i = 0; i < 1000; ++i) {
                r += c[i];
            }
            delete[] c;
            return r + y;
        }
    };
}
