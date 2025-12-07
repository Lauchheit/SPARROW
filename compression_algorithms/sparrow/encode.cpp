#include "sparrow.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <bitset>
#include <algorithm>
#include <cmath>
#include <array>

#include "../../helpers/bit_operations.h"
#include "../../helpers/file_operations.h"
#include "../../input_strategies/source_context.h"
#include "frequency_selection.h"

using namespace std;

const double PI = 3.14159;

int get_leading_zeros(bitset<64> bits){
    unsigned long long val = bits.to_ullong();
    if(val == 0) return 64;
    return __builtin_clzll(val);
}

vector<bool> get_significant_bits(bitset<64> bits, int leading_zeros){
    vector<bool> ret;
    if(leading_zeros >= 64) return ret;
    
    ret.reserve(64 - leading_zeros);
    for(int i = 63 - leading_zeros; i >= 0; i--) {
        ret.push_back(bits[i]);
    }
    
    return ret;
}

vector<bool> get_window_prefix(int window_length, int v){
    int prefix_length = ceil(log2(max(1, window_length)));
    
    vector<bool> result;
    
    for(int i = prefix_length - 1; i >= 0; i--){
        result.push_back(((v >> i) & 1) != 0);
    }
    
    return result;
}

std::vector<bool> SparrowCompression::encode(const SignalContext& signalContext){

    cout << endl << "---------- SPARROW ENCODE ----------" << endl;

    cout << "boop";

    std::vector<double> xs = signalContext.getSignal();

    size_t N = xs.size();

    vector<FrequencyComponent> frequencies = selectOptimalFrequencies(xs, N);
    

    vector<double> x = reconstructSignal(frequencies, N, N);

    cout << "Approximated Frequencies: " << frequencies.size() << endl;
    for(int i=0; i< std::min(frequencies.size(), static_cast<size_t>(10)); i++){
        cout << frequencies[i].frequency << ": " << frequencies[i].amplitude <<" - " << frequencies[i].phase<< endl;
    }

    //Approximate signal

    vector<bitset<64>> x_bit = vector_double_to_bits(x);
    vector<bitset<64>> xs_bit = vector_double_to_bits(xs);

    vector<bitset<64>> r_bit;
    double sum_epsilon = 0;
    for(int i = 0; i < N; i++){
        //cout<<xs[i]<<endl;
        bitset<64> ri = x_bit[i] ^ xs_bit[i];
        r_bit.push_back(ri);

        sum_epsilon += abs(x[i] - xs[i]);

        if(N<=10) cout << endl << i << endl << x_bit[i] << endl << xs_bit[i] << endl << ri << endl;
    }
    cout<< endl << "Average Error: " << sum_epsilon/N << endl;

    array<int,65> first_1_distribution{}; 
    double first_1 = 0;
    for(const auto& r : r_bit)
    {
        bool found = false;
        for(int i = 1; i <= 64; i++){
            int d = r[64-i];
            if(d == 1){
                first_1 += (i-1);
                first_1_distribution[i-1]++;
                found = true;
                break;
            }
        }
        
        if(!found){
            first_1 += 64;
            first_1_distribution[64]++;
        }
    }
    
    int best_wl = -1;
    double min_total_bits = 1e9;

    for(int wl = 1; wl <= 64; wl++){
    double total_bits = 0;
    int prefix_length = ceil(log2(max(1, wl)));
    
        for(int lz = 0; lz <= 64; lz++){
            int count = first_1_distribution[lz];
            if(count == 0) continue;
            
            if(lz >= wl) {
                total_bits += count * (1 + 64 - wl);
            } else {
                total_bits += count * (1 + prefix_length + 64 - lz); 
            }
        }
        
        double avg_bits = total_bits / N;
        
        if(avg_bits < min_total_bits) {
            min_total_bits = avg_bits;
            best_wl = wl;
        }
        
    }

    //cout << "\nOptimal w_l: " << best_wl << endl;
    //cout << "Average bits: " << min_total_bits << endl;
    //cout << "Savings: " << (64.0 - min_total_bits) << " bits" << endl;

    // Actual encoding

    vector<bool> output;
    std::vector<bool> freq_count = int16_to_bitvector(frequencies.size());
    output.insert(output.end(), freq_count.begin(), freq_count.end());

    cout << frequencies.size() << endl;
    for(const auto& f : frequencies){
        vector<bool> freq_bit = double_to_bitvector(f.frequency);
        output.insert(output.end(), freq_bit.begin(), freq_bit.end());

        vector<bool> ampl_bit = double_to_bitvector(f.amplitude);
        output.insert(output.end(), ampl_bit.begin(), ampl_bit.end());

        vector<bool> phase_bit = double_to_bitvector(f.phase);
        output.insert(output.end(), phase_bit.begin(), phase_bit.end());
    }

    vector<bool> best_wl_bits = int16_to_bitvector(best_wl);
    output.insert(output.end(), best_wl_bits.begin(), best_wl_bits.end());

    for (const auto& ri : r_bit){
        bool control_bit;
        vector<bool> prefix;
        vector<bool> significand;
        int leading_zeros =  get_leading_zeros(ri);

        if(leading_zeros >= best_wl){
            control_bit = 1;

            significand = get_significant_bits(ri, best_wl);
        }
        else {
            control_bit = 0;
            prefix = get_window_prefix(best_wl, leading_zeros);
            significand = get_significant_bits(ri, leading_zeros);
        }
        output.push_back(control_bit),
        output.insert(output.end(), prefix.begin(), prefix.end());
        output.insert(output.end(), significand.begin(), significand.end());
    }
    int compression_size = output.size();

    cout  << endl <<"Encoded: " << compression_size << endl<< "Original: " << 64 * N << endl << "Ratio: " << (double)compression_size/(64*N) << endl;
    //write_bitvector_to_file(output);

    return output;
}