#include "stddef.h"
#include "stdint.h"

namespace Task {
    struct Simple {
        static size_t apply(uint16_t x, uint16_t y) {
            return x + y;
        }
    };

    struct Complex {
        static size_t apply(uint16_t x, uint16_t y) {
            size_t result{};

            for (int i = 0; i < x; ++i) {
                for (int j = 0; j < y; ++j) {
                    result += x * x * y + y;
                }
            }

            return result;
        }
    };
}
