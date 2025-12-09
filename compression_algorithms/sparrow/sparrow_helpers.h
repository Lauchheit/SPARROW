#ifndef SPARROW_HELPERS_H
#define SPARROW_HELPERS_H

#include <bitset>
#include <vector>

int get_leading_zeros(std::bitset<64> bits);
std::vector<bool> get_significant_bits(std::bitset<64> bits, int leading_zeros);
std::vector<bool> get_window_prefix(int window_length, int v);

#endif