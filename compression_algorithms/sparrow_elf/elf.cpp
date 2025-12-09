#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <numeric>
#include <cstring>
#include <bitset>
#include <iomanip>
#include "elf.h"
#include "../../helpers/bit_operations.h"
using namespace std;

int getDecimalPlaces(const string& str) {
    size_t pos = str.find('.');
    return (pos == string::npos) ? 0 : str.length() - pos - 1;
}

int getSignificandCount(const string& str) {
    int count = 0;
    bool foundNonZero = false;
    for (char c : str) {
        if (c == '.' || c == '-' || c == '+') continue;
        if (c != '0' || foundNonZero) {
            if (!foundNonZero && c != '0') foundNonZero = true;
            if (foundNonZero) count++;
        }
    }
    return count;
}


uint64_t double_to_uint(double f) {
    uint64_t bits;
    std::memcpy(&bits, &f, sizeof(double));
    return bits;
}

int getExponentFromDouble(double v) {
    uint64_t bits = double_to_uint(v);
    // Extrahiere Bits 52-62 (Exponent)
    int e = (bits >> 52) & 0x7FF;
    return e;
}

int calculateErasurePosition(double v, int decimal_places) {
    int e = getExponentFromDouble(v);
    int f_alpha = ceil(decimal_places * log2(10));
    int g_alpha = f_alpha + e - 1023;
    
    return g_alpha;
}


double LeaveOut(double v_prime, int alpha) {
    double factor = std::pow(10, alpha);
    return std::floor(v_prime * factor) / factor;
}

struct EncodedValue {
    bool wasErased;
    vector<bool> beta_star;
    vector<bool> erased_value;
};

vector<bool> elfEraseResidual(const vector<bool>& residual_bits,
                              int alpha,
                              int beta_star,
                              int erasure_position) {

    if (residual_bits.size() != 64) {
        throw std::invalid_argument("residual_bits must be exactly 64 bits");
    }

    vector<bool> result = residual_bits;

    int bits_to_erase = 52 - erasure_position;
    if (bits_to_erase <= 0) return result;

    // Convert residual to uint64
    bitset<64> temp;
    for (int i = 0; i < 64; i++) {
        temp[63 - i] = residual_bits[i];
    }
    uint64_t residual_uint = temp.to_ullong();

    // Mask ONLY mantissa LSBs
    uint64_t mantissa_mask = ((1ULL << bits_to_erase) - 1);
    uint64_t delta = residual_uint & mantissa_mask;

    if (beta_star < 16 && delta != 0 && bits_to_erase > 4) {
        int erase_start = 64 - bits_to_erase;
        for (int j = erase_start; j < 64; j++) {
            result[64-j] = 0;
        }
    }

    return result;
}

// Vectorized version for multiple residuals
vector<vector<bool>> elfEraseResidualVector(const vector<vector<bool>>& residual_bits,
                                             const vector<int>& alphas,
                                             const vector<int>& beta_stars,
                                             const vector<int>& erasure_positions) {
    size_t N = residual_bits.size();
    
    if (alphas.size() != N || beta_stars.size() != N || erasure_positions.size() != N) {
        throw std::invalid_argument("All input vectors must have the same size");
    }
    
    vector<vector<bool>> results;
    results.reserve(N);
    
    for (size_t i = 0; i < N; i++) {
        results.push_back(elfEraseResidual(residual_bits[i], 
                                           alphas[i], 
                                           beta_stars[i], 
                                           erasure_positions[i]));
    }
    
    return results;
}