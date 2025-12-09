#include "sparrowelf_compression.h"
#include "../../input_strategies/file_string_reader.h"
#include "../sparrow/frequency_selection.h"
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

#include "elf.h"
#include "../sparrow/sparrow_helpers.h"

using namespace std;

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

std::vector<bool> SparrowElfCompression::encode(const std::string& input_filepath){

    cout << endl << "---------- SPARROW ELF ENCODE ----------" << endl;

    FileStringReader reader = {input_filepath};
    vector<string> xs_strings = reader.read();
    vector<double> xs = vector_string_to_double(xs_strings);

    size_t N = xs_strings.size();

    // Calculate ELF metadata for all values
    vector<int> alphas(N);
    vector<int> beta_stars(N);
    vector<int> erasure_positions(N);

    for(int i = 0; i < N; i++) {
        alphas[i] = getDecimalPlaces(xs_strings[i]);
        beta_stars[i] = getSignificandCount(xs_strings[i]);
        erasure_positions[i] = calculateErasurePosition(xs[i], alphas[i]);
    }

    // Frequency selection on original values
    vector<FrequencyComponent> frequencies = selectOptimalFrequencies(xs, N);
    vector<double> x = reconstructSignal(frequencies, N, N);

    /*cout << "Approximated Frequencies: " << frequencies.size() << endl;
    for(int i=0; i< std::min(frequencies.size(), static_cast<size_t>(10)); i++){
        cout << frequencies[i].frequency << ": " << frequencies[i].amplitude <<" | " << frequencies[i].phase<< endl;
    }
    */
    // Compute XOR residuals
    vector<bitset<64>> x_bit = vector_double_to_bits(x);
    vector<bitset<64>> xs_bit = vector_double_to_bits(xs);

    vector<bitset<64>> r_bit;
    double sum_epsilon = 0;
    for(int i = 0; i < N; i++){
        bitset<64> ri = x_bit[i] ^ xs_bit[i];
        r_bit.push_back(ri);
        sum_epsilon += abs(x[i] - xs[i]);

        if(N<=10) cout << endl << i << endl << x_bit[i] << endl << xs_bit[i] << endl << ri << endl;
    }
    cout<< endl << "Average Error: " << sum_epsilon/N << endl;

    // Calculate optimal window length for Sparrow
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

    // === ENCODING ===
    vector<bool> output;
    
    // 1. Frequency count (16 bits)
    std::vector<bool> freq_count = int16_to_bitvector(frequencies.size());
    output.insert(output.end(), freq_count.begin(), freq_count.end());

    // 2. Frequency data (3 × 64 bits each)
    cout << frequencies.size() << endl;
    for(const auto& f : frequencies){
        vector<bool> freq_bit = double_to_bitvector(f.frequency);
        output.insert(output.end(), freq_bit.begin(), freq_bit.end());

        vector<bool> ampl_bit = double_to_bitvector(f.amplitude);
        output.insert(output.end(), ampl_bit.begin(), ampl_bit.end());

        vector<bool> phase_bit = double_to_bitvector(f.phase);
        output.insert(output.end(), phase_bit.begin(), phase_bit.end());
    }

    // 3. Window length (16 bits) - MUST BE HERE, BEFORE RESIDUALS
    vector<bool> best_wl_bits = int16_to_bitvector(best_wl);
    output.insert(output.end(), best_wl_bits.begin(), best_wl_bits.end());

    // 4. ELF + Sparrow encoded residuals
    for (int i = 0; i < r_bit.size(); i++) {
        auto ri = r_bit[i];
        
        // Apply ELF erasure to XOR residual
        vector<bool> ri_bitvec(64);
        for(int j = 0; j < 64; j++) {
            ri_bitvec[j] = ri[j];
        }
        
        vector<bool> ri_erased = elfEraseResidual(ri_bitvec, alphas[i], beta_stars[i], erasure_positions[i]);
        
        // Convert back to bitset for Sparrow
        bitset<64> ri_erased_bitset;
        for(int j = 0; j < 64; j++) {
            ri_erased_bitset[j] = ri_erased[j];
        }
        
        // Calculate ELF metadata
        int bits_to_erase = 52 - erasure_positions[i];
        bool elf_applied = (beta_stars[i] < 16 && bits_to_erase > 4);
        
        uint64_t temp_uint = ri_erased_bitset.to_ullong();
        uint64_t mask = (1ULL << bits_to_erase) - 1;
        uint64_t delta = temp_uint & mask;
        
        if (delta == 0) {
            elf_applied = false;  // Don't apply if no bits would be erased
        }
        
        // Sparrow encoding on ELF-erased residual
        bool sparrow_control_bit;
        vector<bool> sparrow_prefix;
        vector<bool> significand;
        int leading_zeros = get_leading_zeros(ri_erased_bitset);

        if(leading_zeros >= best_wl){
            sparrow_control_bit = 1;
            significand = get_significant_bits(ri_erased_bitset, best_wl);
        }
        else {
            sparrow_control_bit = 0;
            sparrow_prefix = get_window_prefix(best_wl, leading_zeros);
            significand = get_significant_bits(ri_erased_bitset, leading_zeros);
        }
        
        // Write metadata in proper order:
        // [ELF flag][beta_star if flag=1][Sparrow control][Sparrow prefix if control=0][significand]
        
        // ELF metadata (per value)
        output.push_back(elf_applied);
        if (elf_applied) {
            for(int b = 3; b >= 0; b--) {
                output.push_back((beta_stars[i] >> b) & 1);
            }
        }
        
        // Sparrow metadata (per value)
        output.push_back(sparrow_control_bit);
        if (!sparrow_control_bit) {
            output.insert(output.end(), sparrow_prefix.begin(), sparrow_prefix.end());
        }
        
        // Significand data
        output.insert(output.end(), significand.begin(), significand.end());
    }
    
    int compression_size = output.size();

    cout << endl << "Encoded: " << compression_size << endl 
         << "Original: " << 64 * N << endl 
         << "Ratio: " << (double)compression_size/(64*N) << endl;

    return output;
}