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
    if (pos == string::npos) {
        return 0;  // No decimal point
    }
    
    // Find the last non-zero digit after the decimal point
    size_t last_nonzero = pos;
    for (size_t i = str.length() - 1; i > pos; --i) {
        if (str[i] != '0' && str[i] != ' ' && str[i] != '\t') {
            last_nonzero = i;
            break;
        }
    }
    
    // If last_nonzero is still pos, all digits after decimal are zeros
    if (last_nonzero == pos) {
        return 0;
    }
    
    return last_nonzero - pos;
}

int getSignificandCount(const string& str) {
    // First, find the actual significant part (strip trailing zeros after decimal)
    string working_str = str;
    size_t decimal_pos = str.find('.');
    
    if (decimal_pos != string::npos) {
        // Find last non-zero digit
        size_t last_nonzero = decimal_pos;
        for (size_t i = str.length() - 1; i > decimal_pos; --i) {
            if (str[i] != '0' && str[i] != ' ' && str[i] != '\t') {
                last_nonzero = i;
                break;
            }
        }
        // Truncate string to remove trailing zeros
        working_str = str.substr(0, last_nonzero + 1);
    }
    
    int count = 0;
    bool foundNonZero = false;
    
    for (char c : working_str) {
        if (c == '.' || c == '-' || c == '+') continue;
        
        if (c != '0') {
            foundNonZero = true;
        }
        
        if (foundNonZero && (c >= '0' && c <= '9')) {
            count++;
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
    
    return g_alpha + 11;
}

double LeaveOut(double v_prime, int alpha) {
    double factor = std::pow(10, alpha);
    return std::trunc(v_prime * factor) / factor;
}

struct EncodedValue {
    bool wasErased;
    vector<bool> beta_star;
    vector<bool> erased_value;
};