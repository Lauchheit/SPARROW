#ifndef GLOBAL_PARAMS_H
#define GLOBAL_PARAMS_H

#include <cstdint>

namespace GlobalParams {
    constexpr int PRECISION = 64;

    constexpr double FREQUENCY_THRESHOLD = 0.01;
    constexpr int RELATIVE_FREQUENCY_THRESHHOLD = 100000;

    constexpr double BITS_FOR_SAVED_FREQUENCIES = 16;
    constexpr double BITS_FOR_WL = 16;
    
}

#endif // GLOBAL_PARAMS_H