#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <numeric>
#include <cstring>
#include <bitset>
#include <iomanip>
#include "elf_operations.h"
#include "../helpers/bit_operations.h"
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

double string_to_double(const std::string& str) {
    std::string cleaned;
    for (char c : str) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            cleaned += c;
        }
    }
    
    // Replace comma with period (for European decimal notation)
    for (char& c : cleaned) {
        if (c == ',') {
            c = '.';
        }
    }
    
    if (cleaned.empty()) {
        throw std::invalid_argument("Empty string cannot be converted to double");
    }
    
    try {
        size_t pos;
        double value = std::stod(cleaned, &pos);
        
        if (pos != cleaned.length()) {
            throw std::invalid_argument("Invalid characters in string: " + str);
        }
        
        return value;
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Cannot convert to double: " + str);
    } catch (const std::out_of_range&) {
        throw std::out_of_range("Value out of range for double: " + str);
    }
}

vector<double> vector_string_to_double(const vector<string>& string_vector) {
    vector<double> result;
    result.reserve(string_vector.size());
    
    for (const auto& str : string_vector) {
        result.push_back(string_to_double(str));
    }
    
    return result;
}

vector<bool> erasure_pos_to_bitvector(int erasure_pos) {
    std::vector<bool> result;
    result.reserve(6);
    
    for(int i = 5; i >= 0; i--) {
        result.push_back((erasure_pos >> i) & 1);
    }
    
    return result;
}

int bitvector_to_erasure_pos(const std::vector<bool>& bitvec) {
    if(bitvec.size() != 6) {
        throw std::invalid_argument("bitvector must have exactly 6 bits for erasure_pos");
    }
    
    int result = 0;
    
    for(int i = 0; i < 6; i++) {
        result <<= 1;
        if(bitvec[i]) {
            result |= 1;
        }
    }
    
    return result;
}

double elf_reconstruct(double v_prime, int alpha) {
    // Formula from the paper: v = LeaveOut(v', α) + 10^(-α)
    return LeaveOut(v_prime, alpha) + std::pow(10, -alpha) * (v_prime < 0 ? -1 : 1);
}

double elf_reconstruct_roundup_exact(double v_prime, int alpha) {
    int64_t scale = 1;
    for (int i = 0; i < alpha; i++) scale *= 10;

    double scaled = v_prime * scale;

    int64_t rounded;
    if (v_prime >= 0.0) {
        rounded = (int64_t)std::ceil(scaled - 1e-12);
    } else {
        rounded = (int64_t)std::floor(scaled + 1e-12);
    }

    return (double)rounded / scale;
}


uint8_t beta_star_bits_to_uint8(const std::vector<bool>& bitvec) {
    if(bitvec.size() != GlobalParams::BETA_STAR_BITS_SIZE) {
        throw std::invalid_argument("bitvector must have exactly " + 
                                    std::to_string(GlobalParams::BETA_STAR_BITS_SIZE) + 
                                    " bits for beta_star");
    }
    
    uint8_t result = 0;
    
    for(int i = 0; i < GlobalParams::BETA_STAR_BITS_SIZE; i++) {
        result <<= 1;
        if(bitvec[i]) {
            result |= 1;
        }
    }
    
    return result;
}

int8_t calculate_sp(const double v_prime) {
    
    // Handle special cases
    if (v_prime == 0.0 || !std::isfinite(v_prime)) {
        return 0;  // or throw an exception
    }
    
    // SP(v) = floor(log10(|v|))
    // This gives the exponent in scientific notation
    // e.g., 123.456 = 1.23456 × 10^2, so SP = 2
    //       0.00456 = 4.56 × 10^-3, so SP = -3
    int sp = (int)std::floor(std::log10(std::abs(v_prime)));
    
    return sp;
}

uint16_t calculate_alpha(uint8_t beta_star, int sp) {
    // Formula from the paper: α = β - (SP(v) + 1)
    int alpha = beta_star - (sp + 1);
    
    // Ensure alpha is non-negative (shouldn't happen with valid data)
    if (alpha < 0) {
        return 0;
    }
    
    return static_cast<uint8_t>(alpha);
}

bool should_erase(const bitset<64>& value, int erasure_position, int beta){

    auto xs_bitset = value;
    int last_1_in_mantissa = -1;
    for(int j = 12; j < 64; j++) {
        bool is_1 = xs_bitset[63-j];
        if(is_1){
            last_1_in_mantissa = j;
        }
    }
    //cout << endl << "Last 1: " << last_1_in_mantissa << endl;
    //cout << endl << "Approximation: " << x_bit[i] << endl << "Actual Value: " << xs_bit[i] << endl << "XOR: " << r_bit[i]<< endl;        
    
    
    // Calculate ELF metadata
    int bits_to_erase = 52 - erasure_position;

    bool has_erasable_bits = last_1_in_mantissa > erasure_position;
    bool elf_worth = (beta < 16 && bits_to_erase > 4);

    return has_erasable_bits && elf_worth;

}

bool set_beta_star_0(const double value, const int beta_star){
    bool is_power_of_ten = false;
    int power = -1;
    if(std::abs(value) > 0.0 && std::abs(value) <= 1.0 && beta_star == 1) {
        for(int k = 0; k <= 308; ++k) {
            double candidate = std::pow(10.0, -k);
            if (std::abs(std::abs(value) - candidate) < 1e-14) {
                is_power_of_ten = true;
                power = k;
                break;
            }
        }
    }
    return is_power_of_ten;
}