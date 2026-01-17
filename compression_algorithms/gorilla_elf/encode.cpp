#include "gorillaelf_compression.h"

#include <iostream>
#include <vector>
#include <bitset>
#include <algorithm>
#include <cmath>

#include "../../helpers/bit_operations.h"
#include "../../input_strategies/file_string_reader.h"
#include "../elf_operations.h"
#include "../gorilla/gorilla_helpers.h"

using namespace std;


void erase_bitset(bitset<64>& xor_val, const int erasure_pos){
    for(int i = 0; i<64; i++){
        if(i>erasure_pos) {
            xor_val[63-i] = 0;
        }
    }
}

std::vector<bool> GorillaElfCompression::encode(const std::string& input_filepath) {
    cout << endl << "---------- GORILLA ENCODE ----------" << endl;
    
    FileStringReader reader = {input_filepath};
    const vector<string> values_strings = reader.read();
    vector<double> values = vector_string_to_double(values_strings);
    size_t N = values.size();
    
    if(N == 0) {
        return vector<bool>();
    }
    
    cout << "Input values: " << N << endl;
    
    vector<bool> output;
    
    // Encode N (32 bits)
    for(int i = 31; i >= 0; i--) {
        output.push_back((N >> i) & 1);
    }
    

    // First value
    double first_value = values[0];
    string first_value_string = values_strings[0];
    int alpha_0 = getDecimalPlaces(first_value_string);
    uint8_t beta_0 = getSignificandCount(first_value_string);
    int erasure_pos_0 = calculateErasurePosition(first_value, alpha_0);

    bitset<64> first_bits = double_to_bits(first_value);

    if(set_beta_star_0(first_value, beta_0)){
        beta_0=0;
    }

    bool elf_applied_first = should_erase(first_bits, erasure_pos_0, beta_0);

    output.push_back(elf_applied_first);
    if (elf_applied_first) {
        erase_bitset(first_bits, erasure_pos_0);
        // Store beta_0
        for(int b = 3; b >= 0; b--) {
            output.push_back((beta_0 >> b) & 1);
        }
    }

    vector<bool> first_value_bitvector = bitset_to_bitvector(first_bits, 0, 64);
    output.insert(output.end(), first_value_bitvector.begin(), first_value_bitvector.end());

    
    bitset<64> prev_bits_erased = first_bits;

    
    cout << "Encoded header: N=" << N << " first_value=" << values[0] << endl;
    
    if(N == 1) {
        cout << "Only one value, encoded: " << output.size() << " bits" << endl;
        return output;
    }
    
    int case_identical = 0;
    int case_same_window = 0;
    int case_new_window = 0;
    
    MeaningfulWindow current_window;
    bool window_initialized = false;
    
    // Process remaining values
    for(size_t i = 1; i < N; i++) {
        double curr_double = values[i];
        bitset<64> curr_bits = double_to_bits(curr_double);

        string curr_string = values_strings[i];
        int alpha = getDecimalPlaces(curr_string);
        uint8_t beta = getSignificandCount(curr_string);

        if(set_beta_star_0(curr_double, beta)){
            beta=0;
        }

        int erasure_pos = calculateErasurePosition(curr_double, alpha);


        bool elf_applied = should_erase(curr_bits, erasure_pos, beta);

        output.push_back(elf_applied);

        if (elf_applied) {
            erase_bitset(curr_bits, erasure_pos);
            for(int b = 3; b >= 0; b--) {
                output.push_back((beta >> b) & 1);
                //cout << ((beta_stars[i] >> b) & 1);
            }
        }

        bitset<64> xor_val = prev_bits_erased  ^ curr_bits;
        prev_bits_erased = curr_bits;

        
        // Case 0: Identical
        if(xor_val.to_ullong() == 0) {
            output.push_back(0);
            case_identical++;
            
            if(i < 20 || (i % 1000 == 0)) {
                cout << "i=" << i << " IDENTICAL (control bit: 0)" << endl;
            }
            continue;
        }
        
        // Not identical - write control bit 1
        output.push_back(1);

        
        MeaningfulWindow new_window = get_meaningful_window(xor_val);
        
        // First non-identical OR doesn't fit in window
        if(!window_initialized || !window_fits_in_previous(new_window, current_window)) {
            output.push_back(1); // Control bit 2: New window
            current_window = new_window;
            window_initialized = true;
            
            if(i < 50 || case_new_window < 10 || (i % 1000 == 0)) {
                cout << "i=" << i << " NEW WINDOW (control bits: 11)" << endl;
                cout << "  lz=" << current_window.leading_zeros 
                     << " mb=" << current_window.meaningful_bits 
                     << " tz=" << current_window.trailing_zeros 
                     << " sum=" << (current_window.leading_zeros + current_window.meaningful_bits + current_window.trailing_zeros)
                     << endl;
            }
            
            vector<bool> window_info = encode_window_info(current_window);
            output.insert(output.end(), window_info.begin(), window_info.end());
            
            vector<bool> meaningful = extract_meaningful_bits(xor_val, current_window);
            output.insert(output.end(), meaningful.begin(), meaningful.end());
            
            case_new_window++;
        }
        // Fits in existing window
        else {
            output.push_back(0); // Control bit 2: Use previous window
            
            if(i < 20 || (case_same_window < 10)) {
                cout << "i=" << i << " SAME WINDOW (control bits: 10) mb=" 
                     << current_window.meaningful_bits << endl;
            }
            
            vector<bool> meaningful = extract_meaningful_bits(xor_val, current_window);
            output.insert(output.end(), meaningful.begin(), meaningful.end());
            
            case_same_window++;
        }

    }
    
    int original_bits = 64 * N;
    int compressed_bits = output.size();
    
    cout << endl << "Compression Statistics:" << endl;
    cout << "Original bits: " << original_bits << endl;
    cout << "Compressed bits: " << compressed_bits << endl;
    cout << "Compression ratio: " << (double)compressed_bits / original_bits << endl;
    cout << endl;
    cout << "Case 0 (identical values): " << case_identical << endl;
    cout << "Case A (same window): " << case_same_window << endl;
    cout << "Case B (new window): " << case_new_window << endl;
    
    return output;
}