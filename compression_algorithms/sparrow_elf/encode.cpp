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



std::vector<bool> erasure_pos_to_bitvector(int erasure_pos) {
    /*if(erasure_pos < 0 || erasure_pos > 52 + 11) {
        throw std::invalid_argument("erasure_pos must be between 0 and 52");
    }*/
    
    std::vector<bool> result;
    result.reserve(6);
    
    for(int i = 5; i >= 0; i--) {
        result.push_back((erasure_pos >> i) & 1);
    }
    
    return result;
}

std::vector<bool> SparrowElfCompression::encode(const std::string& input_filepath){

    cout << endl << "---------- SPARROW ELF ENCODE ----------" << endl;

    FileStringReader reader = {input_filepath};
    const vector<string> xs_strings = reader.read();
    vector<double> xs = vector_string_to_double(xs_strings);

    uint64_t N = xs_strings.size();

    // Calculate ELF metadata for all values
    vector<int> alphas(N);
    vector<int> beta_stars(N);
    vector<int> erasure_positions(N);

    for(int i = 0; i < N; i++) {
        if (!std::isfinite(xs[i]) || std::abs(xs[i]) == 0.0) {
            alphas[i] = 0;
            beta_stars[i] = 0;
            erasure_positions[i] = 52;  // Don't erase
            continue;
        }
        
        int beta_star = getSignificandCount(xs_strings[i]);
        // Check if xs[i] is exactly 10^-k for some integer k
        bool is_power_of_ten = false;
        int power = -1;
        if(std::abs(xs[i]) > 0.0 && std::abs(xs[i]) <= 1.0 && beta_star == 1) {
            for(int k = 0; k <= 308; ++k) {
                double candidate = std::pow(10.0, -k);
                if (std::abs(std::abs(xs[i]) - candidate) < 1e-14) {
                    is_power_of_ten = true;
                    power = k;
                    break;
                }
            }
}
        if(is_power_of_ten) {
            beta_star = 0;
        }
        alphas[i] = getDecimalPlaces(xs_strings[i]);
        beta_stars[i] = beta_star;
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

    cout << "Best Wl: " << best_wl << endl;

    // === ENCODING ===
    vector<bool> output;

    // 1. Frequency count (16 bits)
    std::vector<bool> freq_count = int16_to_bitvector(frequencies.size());
    output.insert(output.end(), freq_count.begin(), freq_count.end());

    // 2. Frequency data (3 × 64 bits each)
    cout << "Saved frequencies: " << frequencies.size() << endl;
    for(const auto& f : frequencies){
        vector<bool> freq_bit = double_to_bitvector(f.frequency);
        output.insert(output.end(), freq_bit.begin(), freq_bit.end());

        vector<bool> ampl_bit = double_to_bitvector(f.amplitude);
        output.insert(output.end(), ampl_bit.begin(), ampl_bit.end());

        vector<bool> phase_bit = double_to_bitvector(f.phase);
        output.insert(output.end(), phase_bit.begin(), phase_bit.end());
    }

    // 3. Window length (16 bits)
    vector<bool> best_wl_bits = int16_to_bitvector(best_wl);
    output.insert(output.end(), best_wl_bits.begin(), best_wl_bits.end());

    // ADD N with 
    vector<bool> n_bits = uint64_to_bitvector(N);
    output.insert(output.end(), n_bits.begin(), n_bits.end());
    cout << "Number of samples: " << N << endl;

    // 4. ELF + Sparrow encoded residuals
    for (int i = 0; i < r_bit.size(); i++) {
        cout <<endl<< "-----------" << endl << "idx0:"<<i<<"."<<endl;
        auto ri = r_bit[i];

        cout << endl<< "Number: "<< xs_strings[i] << endl << "Alpha: "<< alphas[i] << endl << "Beta: " << beta_stars[i] << endl << "Erasure Pos: " << erasure_positions[i] <<endl;
       

        //zero control bit 
        if(xs[i] == 0.0){
            output.push_back(0);
            cout << " zc 0" << endl;
            continue;
        }
        output.push_back(1);
        cout << " zc 1";

        
        // Apply ELF erasure to XOR residual
        //vector<bool> ri_bitvec(64);
        auto xs_bitset = xs_bit[i];
        int last_1_in_mantissa = -1;
        for(int j = 12; j < 64; j++) {
            bool is_1 = xs_bitset[63-j];
            if(is_1){
                last_1_in_mantissa = j;
            }
        }
        cout << endl << "Last 1: " << last_1_in_mantissa << endl;
        cout << endl << "Approximation: " << x_bit[i] << endl << "Actual Value: " << xs_bit[i] << endl << "XOR: " << r_bit[i]<< endl;        
        
        
        // Calculate ELF metadata
        int bits_to_erase = 52 - erasure_positions[i];

        bool has_erasable_bits = last_1_in_mantissa > erasure_positions[i];
        bool elf_worth = (beta_stars[i] < 16 && bits_to_erase > 4);

        bool elf_applied = has_erasable_bits && elf_worth;

        // Sparrow encoding on ELF-erased residual
        bool sparrow_control_bit;
        vector<bool> sparrow_prefix;
        vector<bool> significand;
        int leading_zeros = get_leading_zeros(ri);

        if(leading_zeros >= best_wl){
            sparrow_control_bit = 1;
            significand = get_significant_bits(ri, best_wl);
        }
        else {
            sparrow_control_bit = 0;
            sparrow_prefix = get_window_prefix(best_wl, leading_zeros);
            significand = get_significant_bits(ri, leading_zeros);
        }

        cout << endl;
        // ELF metadata (per value)
        output.push_back(elf_applied);
        cout << endl << "ec " << elf_applied << " " << " eb* ";
        if (elf_applied) {
            for(int b = 3; b >= 0; b--) {
                output.push_back((beta_stars[i] >> b) & 1);
                cout << ((beta_stars[i] >> b) & 1);
            }
        }

        
        // Sparrow metadata (per value)
        output.push_back(sparrow_control_bit);
        cout << " sc " << sparrow_control_bit;
        cout << " sp ";
        if (!sparrow_control_bit) {
            print_bitvector(sparrow_prefix);

            output.insert(output.end(), sparrow_prefix.begin(), sparrow_prefix.end());
        }

        cout << endl << "Significand before removing trailing 0s: " << endl; print_bitvector(significand); cout << endl;

        int last_1_in_residual = -1;
        for(int j = significand.size()-1; j >= 0; j--) {
            if(significand[j]) {
                last_1_in_residual = j;
                break;
            }
        }

        cout << "Last 1 in res: " << last_1_in_residual << endl;

        // Calculate data length based on whether ELF is applied
        int data_length;
        if(!xs[i]) {
            data_length = 0;
        } else if (elf_applied) {
            // When ELF is applied, write up to erasure position
            int leading_offset = sparrow_control_bit ? best_wl : leading_zeros;
            int absolute_erasure_pos = erasure_positions[i];
            data_length = absolute_erasure_pos - leading_offset + 1;

            if (data_length < 0) {
                data_length = 0;
            }

            // Make sure we don't exceed significand size
            data_length = min((int)data_length, (int)significand.size());
        } else {
            if (last_1_in_residual < 0) {
                data_length = 0;
            } else {
                data_length = last_1_in_residual + 1;
            }
        }

        cout << endl << "d size: " << data_length << endl;
        
        vector<bool> meaningful_bitset_length = erasure_pos_to_bitvector(data_length);
        cout << " eep "; print_bitvector(meaningful_bitset_length);

        output.insert(output.end(), meaningful_bitset_length.begin(), meaningful_bitset_length.end());
        
        // Significand data
        vector<bool> kept_significand(
            significand.begin(),
            significand.begin() + (size_t)data_length
        );
        
        output.insert(output.end(), kept_significand.begin(), kept_significand.end() );

        cout << " d "; 
        print_bitvector(kept_significand);
        cout << endl << "Leading 0s: " << leading_zeros << endl;
    }
    
    int compression_size = output.size();

    cout << endl << "Encoded: " << compression_size << endl 
         << "Original: " << 64 * N << endl 
         << "Ratio: " << (double)compression_size/(64*N) << endl;


    cout << endl << endl;

    return output;
}