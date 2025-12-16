#ifndef ELF_OPERATIONS_H
#define ELF_OPERATIONS_H

#include <vector>
#include <string>
#include <bitset>
#include <cstdint>
#include "../global_parameters.h"

int getDecimalPlaces(const std::string& str); //alpha
int getSignificandCount(const std::string& str); //beta
uint64_t double_to_uint(double f);
int getExponentFromDouble(double v);
int calculateErasurePosition(double v, int decimal_places);
double LeaveOut(double v_prime, int alpha);
double string_to_double(const std::string& str);
std::vector<double> vector_string_to_double(const std::vector<std::string>& string_vector);
std::vector<bool> erasure_pos_to_bitvector(int erasure_pos);
int bitvector_to_erasure_pos(const std::vector<bool>& bitvec);
double elf_reconstruct(double v_prime, int alpha);
double elf_reconstruct_roundup_exact(double v_prime, int alpha);
uint8_t beta_star_bits_to_uint8(const std::vector<bool>& bitvec);
int8_t calculate_sp(const double v_prime);
uint16_t calculate_alpha(uint8_t beta_star, int sp);
bool should_erase(const std::bitset<64>& value, int erasure_position, int beta);
bool set_beta_star_0(const double value, const int beta_star);

#endif