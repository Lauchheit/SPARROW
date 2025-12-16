#include "gorillaelf_compression.h"

#include <iostream>
#include <vector>
#include <bitset>
#include <cmath>

#include "../../input_strategies/binary_reader.h"
#include "../../helpers/bit_operations.h"
#include "../elf_operations.h"
#include "../gorilla/gorilla_helpers.h"

using namespace std;

std::vector<double> GorillaElfCompression::decode(const BinaryFileReader& reader) {
    cout << endl << "---------- GORILLA DECODE ----------" << endl;
    
    vector<bool> bits = reader.getSignalCode();
    cout << "Total bits read: " << bits.size() << endl;

    string check_filepath = "C:\\Users\\cleme\\OneDrive\\uni\\Informatik\\Bachelorarbeit\\code\\SPARROW\\data\\signal_data.txt";
    SignalContext context(std::make_unique<FileSignalStrategy>(check_filepath));
    std::vector<double> check = context.getSignal();
    
    if(bits.size() < 96) { // 32 for N + 64 for first value
        cerr << "Error: Not enough bits for header and first value" << endl;
        return vector<double>();
    }
    
    vector<double> decoded_values;
    size_t pos = 0;
    
    // Read N (32 bits)
    uint32_t N = 0;
    for(int i = 0; i < 32; i++) {
        N <<= 1;
        if(bits[pos++]) {
            N |= 1;
        }
    }
    cout << "Number of values to decode: " << N << endl;
    
    // Read first value (64 bits)
    bool elf_control_bit_first = bits[pos++];
    uint8_t beta_star_first = 0;
    if(elf_control_bit_first) {
        vector<bool> beta_star_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::BETA_STAR_BITS_SIZE);
        pos += 4;
        beta_star_first = beta_star_bits_to_uint8(beta_star_bits);
    }

    // Read first value (64 bits) - this is already erased
    vector<bool> first_value_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::PRECISION);
    pos += 64;
    double first_value_prime = bitvector_to_double(first_value_bits);

    // Restore first value
    double first_value = first_value_prime;  
    if(elf_control_bit_first) {
        int8_t sp = calculate_sp(first_value_prime);
        uint16_t alpha = calculate_alpha(beta_star_first, sp);
        first_value = elf_reconstruct_roundup_exact(first_value_prime, alpha);
    }

    decoded_values.push_back(first_value);
    bitset<64> prev_bits = double_to_bits(first_value_prime);
    
    cout << "First value decoded: " << first_value << endl;
    cout << "Starting position after header: " << pos << endl << endl;
    
    if(N == 1) {
        cout << "Only one value in stream" << endl;
        return decoded_values;
    }
    
    // For tracking the current window
    DecodeMeaningfulWindow current_window;
    bool window_initialized = false;
    
    // Statistics
    int case_identical = 0;
    int case_same_window = 0;
    int case_new_window = 0;
    
    // Decode remaining values
    bitset<64> prev_bits_erased = prev_bits;
    size_t iter = 1;
    while(iter < N && pos < bits.size()) {
        
        
        // Progress output
        if(iter < 50 || (iter % 1000 == 0)) {
            cout << "--- Iteration " << iter << " (pos=" << pos << ") ---" << endl;
        }
        
        // Check if we have at least one control bit
        if(pos >= bits.size()) {
            cerr << "ERROR: Unexpected end of data at position " << pos << endl;
            break;
        }
        
        bool elf_control_bit = bits[pos++];

        uint8_t beta_star = 0;
        if(elf_control_bit){
            vector<bool> beta_star_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::BETA_STAR_BITS_SIZE);
            pos += GlobalParams::BETA_STAR_BITS_SIZE;
            print_bitvector(beta_star_bits);

            beta_star = beta_star_bits_to_uint8(beta_star_bits);
        }

        // Read first control bit
        bool gorilla_control_bit_1 = bits[pos++];
        
        if(iter < 50 || (iter % 1000 == 0)) {
            cout << "Control bit 1: " << gorilla_control_bit_1;
        }
        
        bitset<64> xor_val(0);

        
        // Case 0: Identical to previous value
        if(!gorilla_control_bit_1) {
            // XOR is all zeros, so current value equals previous value
            if(iter < 50 || (iter % 1000 == 0)) {
                cout << " -> IDENTICAL" << endl;
            }
            xor_val = bitset<64>(0);
            case_identical++;
        }
        else {
            // Not identical, read second control bit
            if(pos >= bits.size()) {
                cerr << "ERROR: Incomplete data at position " << pos << endl;
                break;
            }
            
            bool control_bit_2 = bits[pos++];
            
            if(iter < 50 || (iter % 1000 == 0)) {
                cout << ", Control bit 2: " << control_bit_2;
            }
            
            // Case A: Use previous window
            if(!control_bit_2) {
                if(iter < 50 || (iter % 1000 == 0)) {
                    cout << " -> SAME WINDOW (mb=" << current_window.meaningful_bits << ")" << endl;
                }
                
                if(!window_initialized) {
                    cerr << "ERROR: Trying to use previous window but no window initialized yet!" << endl;
                    break;
                }
                
                // Read meaningful bits using current window
                if(pos + current_window.meaningful_bits > bits.size()) {
                    cerr << "ERROR: Not enough bits for meaningful data at position " << pos 
                         << " (need " << current_window.meaningful_bits << " bits)" << endl;
                    break;
                }
                
                vector<bool> meaningful_bits(bits.begin() + pos, 
                                             bits.begin() + pos + current_window.meaningful_bits);
                pos += current_window.meaningful_bits;
                
                xor_val = reconstruct_from_meaningful_bits(meaningful_bits, current_window);
                case_same_window++;
            }
            // Case B: New window
            else {
                if(iter < 50 || case_new_window < 10 || (iter % 1000 == 0)) {
                    cout << " -> NEW WINDOW" << endl;
                }
                
                // Read new window information (5 + 6 = 11 bits)
                if(pos + 11 > bits.size()) {
                    cerr << "ERROR: Not enough bits for window info at position " << pos << endl;
                    break;
                }
                
                current_window = decode_window_info(bits, pos);
                window_initialized = true;
                
                // Validate window
                if(current_window.trailing_zeros < 0) {
                    cerr << "FATAL ERROR: Negative trailing_zeros=" << current_window.trailing_zeros << endl;
                    break;
                }
                
                // Read meaningful bits
                if(pos + current_window.meaningful_bits > bits.size()) {
                    cerr << "ERROR: Not enough bits for meaningful data at position " << pos 
                         << " (need " << current_window.meaningful_bits << " bits)" << endl;
                    break;
                }
                
                vector<bool> meaningful_bits(bits.begin() + pos, 
                                             bits.begin() + pos + current_window.meaningful_bits);
                pos += current_window.meaningful_bits;
                
                xor_val = reconstruct_from_meaningful_bits(meaningful_bits, current_window);
                case_new_window++;
            }
        }
        
        // XOR with previous value to get current value
        bitset<64> curr_bits_erased = prev_bits_erased ^ xor_val;

        double curr_value;
        if(elf_control_bit){
            double curr_value_prime = bitset_to_double(curr_bits_erased);
            int8_t significant_position = calculate_sp(curr_value_prime);
            uint16_t alpha = calculate_alpha(beta_star, significant_position);

            curr_value = elf_reconstruct_roundup_exact(curr_value_prime, alpha);
        }
        else{
            curr_value = bitset_to_double(curr_bits_erased);
        }

        if(iter < 20 || (iter % 1000 == 0)) {
            cout << "  Decoded value: " << curr_value << endl;
        }
        
        prev_bits_erased = curr_bits_erased;
        decoded_values.push_back(curr_value);

        if(curr_value != check[iter]){
            cout << endl << "Mismatch: " 
                 << endl << "Reconstruction: " <<  curr_value 
                 << endl << "      Original: " << check[iter] 
                 << endl;
        }
        iter++;
    }
    
    cout << endl << "==================================" << endl;
    cout << "Decoding complete!" << endl;
    cout << "Decoded " << decoded_values.size() << " values (expected " << N << ")" << endl;
    cout << "Final position: " << pos << " / " << bits.size() << " bits" << endl;
    
    int remaining = bits.size() - pos;
    if(remaining > 7) {
        cerr << "WARNING: " << remaining << " bits remaining (should be < 8 for padding)" << endl;
    } else {
        cout << "Padding: " << remaining << " bits (OK)" << endl;
    }
    
    cout << endl << "Decode Statistics:" << endl;
    cout << "Case 0 (identical values): " << case_identical << endl;
    cout << "Case A (same window): " << case_same_window << endl;
    cout << "Case B (new window): " << case_new_window << endl;
    cout << "==================================" << endl;
    
    if(decoded_values.size() != N) {
        cerr << "ERROR: Decoded count mismatch! Got " << decoded_values.size() 
             << " but expected " << N << endl;
    }
    
    return decoded_values;
}