#ifndef GLOBAL_PARAMS_H
#define GLOBAL_PARAMS_H

#include <cstdint>

namespace GlobalParams {
    constexpr int PRECISION = 64;

    constexpr double FREQUENCY_THRESHOLD = 0.01;
    constexpr int RELATIVE_FREQUENCY_THRESHHOLD = 100000;

    constexpr size_t BITS_FOR_SAVED_FREQUENCIES = 16;
    constexpr size_t BITS_FOR_WL = 16;
    
    constexpr size_t BETA_STAR_BITS_SIZE = 4;
    constexpr size_t MEANINGFUL_BITS_SIZE_ELF = 6; //ceil(log2(52)) since max is 52

    constexpr size_t BITS_FOR_N_DATA_POINTS = 64;
}

#endif // GLOBAL_PARAMS_H