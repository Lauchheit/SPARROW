#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <numeric>
#include <cstring>
#include <bitset>
#include <iomanip>
#include <fstream>
using namespace std;


bitset<64> double_to_bits(double f) {
    uint64_t bits;
    std::memcpy(&bits, &f, sizeof(double));
    return bits;
}

vector<bitset<64>> vector_double_to_bits(const vector<double>& x){
    vector<bitset<64>> ret(x.size());  // Pre-size with default values
    for(size_t i = 0; i < x.size(); i++){
        ret[i] = double_to_bits(x[i]); 
    }
    return ret;
}

double bitset_to_double(const std::bitset<64>& bits) {
    uint64_t as_uint = bits.to_ullong();
    double result;
    std::memcpy(&result, &as_uint, sizeof(double));
    return result;
}

void printBits(double f, const string& label = "") {
    bitset<64> bits = double_to_bits(f);
    if (!label.empty()) {
        cout << label << ": ";
    }
    cout << (bits) << " (Double: " << f << ")" << endl;
}


void append_bits(std::vector<bool>& vec, const std::string& bits) {
    for(char c : bits) {
        vec.push_back(c == '1');
    }
}

std::vector<bool> double_to_bitvector(double d) {
    std::bitset<64> bits = double_to_bits(d);
    std::vector<bool> result;
    result.reserve(64);
    
    for(int i = 63; i >= 0; i--) {
        result.push_back(bits[i]);
    }
    
    return result;
}

std::vector<bool> int16_to_bitvector(uint16_t value) {
    std::bitset<16> bits(value);
    std::vector<bool> result;
    result.reserve(16);
    
    for(int i = 15; i >= 0; i--) {
        result.push_back(bits[i]);
    }
    
    return result;
}

std::vector<bool> bitset_to_bitvector(const std::bitset<64>& bits, int start, int length) {
    std::vector<bool> result;
    std::string str = bits.to_string();
    for(int i = start; i < start + length && i < 64; i++) {
        result.push_back(str[i] == '1');
    }
    return result;
}


uint16_t bitvector_to_uint16(const std::vector<bool>& bitvec) {
    if(bitvec.size() != 16) {
        throw std::invalid_argument("bitvector must have exactly 16 bits for uint16");
    }
    
    uint16_t result = 0;
    
    for(int i = 0; i < 16; i++) {
        result <<= 1;
        if(bitvec[i]) {
            result |= 1;
        }
    }
    
    return result;
}

double bitvector_to_double(const std::vector<bool>& bitvec) {
    if(bitvec.size() != 64) {
        throw std::invalid_argument("bitvector must have exactly 64 bits for double");
    }
    
    std::bitset<64> bits;
    for(int i = 0; i < 64; i++) {
        bits[63 - i] = bitvec[i];
    }
    return bitset_to_double(bits);
}

std::bitset<64> bitvector_to_bitset64(const std::vector<bool>& bitvec) {
    if(bitvec.size() != 64) {
        throw std::invalid_argument("bitvector must have exactly 64 bits for bitset<64>");
    }
    
    std::bitset<64> result;
    for(int i = 0; i < 64; i++) {
        result[63 - i] = bitvec[i];
    }
    
    return result;
}

void print_bitvector(const std::vector<bool>& bits) {
    for (bool bit : bits) {
        std::cout << (bit ? '1' : '0');
    }
}