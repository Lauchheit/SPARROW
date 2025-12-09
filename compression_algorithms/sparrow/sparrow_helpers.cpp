#include "sparrow_helpers.h"
#include <cmath>
#include <algorithm>

int get_leading_zeros(std::bitset<64> bits) {
    unsigned long long val = bits.to_ullong();
    if(val == 0) return 64;
    return __builtin_clzll(val);
}

std::vector<bool> get_significant_bits(std::bitset<64> bits, int leading_zeros) {
    std::vector<bool> ret;
    if(leading_zeros >= 64) return ret;
    
    ret.reserve(64 - leading_zeros);
    for(int i = 63 - leading_zeros; i >= 0; i--) {
        ret.push_back(bits[i]);
    }
    
    return ret;
}

std::vector<bool> get_window_prefix(int window_length, int v) {
    int prefix_length = std::ceil(std::log2(std::max(1, window_length)));
    
    std::vector<bool> result;
    
    for(int i = prefix_length - 1; i >= 0; i--){
        result.push_back(((v >> i) & 1) != 0);
    }
    
    return result;
}