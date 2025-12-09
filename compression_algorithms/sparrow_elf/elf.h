#ifndef ELF_H
#define ELF_H

#include <vector>
#include <string>
#include <bitset>
#include <cstdint>

int getDecimalPlaces(const std::string& str);
int getSignificandCount(const std::string& str);
uint64_t double_to_uint(double f);
int getExponentFromDouble(double v);
int calculateErasurePosition(double v, int decimal_places);
double LeaveOut(double v_prime, int alpha);
std::vector<bool> elfEraseResidual(const std::vector<bool>& residual_bits, 
                                    int alpha, 
                                    int beta_star, 
                                    int erasure_position);
std::vector<std::vector<bool>> elfEraseResidualVector(
    const std::vector<std::vector<bool>>& residual_bits,
    const std::vector<int>& alphas,
    const std::vector<int>& beta_stars,
    const std::vector<int>& erasure_positions);

#endif