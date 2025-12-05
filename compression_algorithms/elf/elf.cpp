#include <iostream>
#include <vector>
#include <string>
#include <bitset>

#include "bit_operations.cpp"
using namespace std;

int getExponentFromDouble(double v) {
    uint64_t bits = double_to_bits(v);
    // Extrahiere Bits 52-62 (Exponent)
    int e = (bits >> 52) & 0x7FF;
    return e;
}

int calculateErasurePosition(double v, int decimal_places) {
    int e = getExponentFromDouble(v);
    int f_alpha = ceil(decimal_places * log2(10));
    int g_alpha = f_alpha + e - 1023;
    
    // Position im Mantissen-Array (0-51)
    return g_alpha;
}

double LeaveOut(double v_prime, int alpha) {
    double factor = std::pow(10, alpha);
    return std::floor(v_prime * factor) / factor;
}

double bitsetToDouble(const bitset<64>& b) {
    uint64_t bits = b.to_ullong();
    return bitsToDouble(bits);
}


vector<bitset<64>> elf_encode(const vector<double>& x){
    vector<bitset<64>> ret;
    double error = 0;
    
    
    for(int i=0; i<x.size(); i++){
        double v = x.at(i);
        bitset<64> b = bitset<64>(double_to_bits(v));

        int a = 2;
        int f = ceil(abs(log2(pow(10, (-a)))));
        int erasure_pos = calculateErasurePosition(v, a);

        int control_bit;
        
        if(erasure_pos < -12) {
            control_bit = 0;
            
            ret.push_back(v);
            continue;
        }
        
        int start_j = 12 + erasure_pos;
        
        for(int j = start_j; j<64; j++){
            if(63-j < 0 || 63-j >= 64) {
                continue;
            }
            int d = b[63 - j];
            b[63 - j] = 0;
        }
        
        ret.push_back(b);

        double rounded = bitsetToDouble(b);
        double reconstruction = LeaveOut(rounded, a) + pow(10,-a);
        error += abs(reconstruction - x[i]);
    }
    
    //cout << "elf completed successfully" << endl;
    //cout << "Average Error: " << error/x.size() << endl;
    return ret;
}