#ifndef BIT_OPERATIONS_H
#define BIT_OPERATIONS_H

#include <bitset>
#include <vector>
#include <string>
#include <cstdint>

std::bitset<64> double_to_bits(double f);
std::vector<std::bitset<64>> vector_double_to_bits(const std::vector<double>& x);
double bitset_to_double(const std::bitset<64>& bits);
void printBits(double f, const std::string& label = "");
void append_bits(std::vector<bool>& vec, const std::string& bits);
std::vector<bool> double_to_bitvector(double d);
std::vector<bool> int16_to_bitvector(uint16_t value);
std::vector<bool> bitset_to_bitvector(const std::bitset<64>& bits, int start, int length);
uint16_t bitvector_to_uint16(const std::vector<bool>& bitvec);
double bitvector_to_double(const std::vector<bool>& bitvec);
std::bitset<64> bitvector_to_bitset64(const std::vector<bool>& bitvec);

#endif // BIT_OPERATIONS_H