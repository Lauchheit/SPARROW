#include "gorilla.h"

#include <iostream>
#include <vector>
#include <bitset>
#include <cmath>

#include "../../input_strategies/binary_reader.h"
#include "../../helpers/bit_operations.h"

using namespace std;

// Helper structure for window information
struct DecodeMeaningfulWindow {
    int leading_zeros;
    int meaningful_bits;
    int trailing_zeros;
    
    DecodeMeaningfulWindow() : leading_zeros(0), meaningful_bits(0), trailing_zeros(0) {}
    
    DecodeMeaningfulWindow(int lz, int mb) : leading_zeros(lz), meaningful_bits(mb) {
        trailing_zeros = 64 - leading_zeros - meaningful_bits;
    }
};

// Decode window information from bit stream
DecodeMeaningfulWindow decode_window_info(const vector<bool>& bits, size_t& pos) {
    // Read 5 bits for leading zeros
    int leading_zeros = 0;
    for(int i = 0; i < 5; i++) {
        leading_zeros <<= 1;
        if(bits[pos++]) {
            leading_zeros |= 1;
        }
    }
    
    // Read 6 bits for meaningful bits length
    int meaningful_bits = 0;
    for(int i = 0; i < 6; i++) {
        meaningful_bits <<= 1;
        if(bits[pos++]) {
            meaningful_bits |= 1;
        }
    }
    
    // CRITICAL FIX: We stored (mb - 1), so add 1 back
    // This maps 0-63 back to 1-64
    meaningful_bits = meaningful_bits + 1;
    
    cout << "  Decoded window: lz=" << leading_zeros 
         << " mb=" << meaningful_bits 
         << " tz=" << (64 - leading_zeros - meaningful_bits) << endl;
    
    // Validate the window
    if(leading_zeros + meaningful_bits > 64) {
        cerr << "ERROR: Invalid decoded window - lz=" << leading_zeros 
             << " + mb=" << meaningful_bits << " = " << (leading_zeros + meaningful_bits)
             << " > 64" << endl;
        // Attempt recovery
        meaningful_bits = 64 - leading_zeros;
    }
    
    return DecodeMeaningfulWindow(leading_zeros, meaningful_bits);
}

// Reconstruct bitset from meaningful bits and window information
bitset<64> reconstruct_from_meaningful_bits(const vector<bool>& meaningful_bits, 
                                            const DecodeMeaningfulWindow& window) {
    bitset<64> result(0);
    
    if(meaningful_bits.empty()) {
        return result;
    }
    
    // Validate sizes match
    if(meaningful_bits.size() != (size_t)window.meaningful_bits) {
        cerr << "WARNING: meaningful_bits size mismatch! Expected " 
             << window.meaningful_bits << " but got " << meaningful_bits.size() << endl;
    }
    
    // Start from the MSB position of the meaningful window
    // and place bits going downward
    int bit_position = 63 - window.leading_zeros;
    
    for(size_t i = 0; i < meaningful_bits.size(); i++) {
        if(bit_position < window.trailing_zeros) {
            cerr << "ERROR: bit_position " << bit_position 
                 << " below trailing_zeros " << window.trailing_zeros << endl;
            break;
        }
        if(bit_position >= 64 || bit_position < 0) {
            cerr << "ERROR: bit_position " << bit_position << " out of range [0,63]" << endl;
            break;
        }
        result[bit_position] = meaningful_bits[i];
        bit_position--;
    }
    
    return result;
}

std::vector<double> GorillaCompression::decode(const BinaryFileReader& reader) {
    cout << endl << "---------- GORILLA DECODE ----------" << endl;
    
    vector<bool> bits = reader.getSignalCode();
    cout << "Total bits read: " << bits.size() << endl;
    
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
    vector<bool> first_value_bits(bits.begin() + pos, bits.begin() + pos + 64);
    pos += 64;
    double first_value = bitvector_to_double(first_value_bits);
    decoded_values.push_back(first_value);
    
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
    bitset<64> prev_bits = double_to_bits(decoded_values.back());
    
    while(decoded_values.size() < N && pos < bits.size()) {
        size_t iteration = decoded_values.size();
        
        // Progress output
        if(iteration < 50 || (iteration % 1000 == 0)) {
            cout << "--- Iteration " << iteration << " (pos=" << pos << ") ---" << endl;
        }
        
        // Check if we have at least one control bit
        if(pos >= bits.size()) {
            cerr << "ERROR: Unexpected end of data at position " << pos << endl;
            break;
        }
        
        // Read first control bit
        bool control_bit_1 = bits[pos++];
        
        if(iteration < 50 || (iteration % 1000 == 0)) {
            cout << "Control bit 1: " << control_bit_1;
        }
        
        bitset<64> xor_val(0);
        
        // Case 0: Identical to previous value
        if(!control_bit_1) {
            // XOR is all zeros, so current value equals previous value
            if(iteration < 50 || (iteration % 1000 == 0)) {
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
            
            if(iteration < 50 || (iteration % 1000 == 0)) {
                cout << ", Control bit 2: " << control_bit_2;
            }
            
            // Case A: Use previous window
            if(!control_bit_2) {
                if(iteration < 50 || (iteration % 1000 == 0)) {
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
                if(iteration < 50 || case_new_window < 10 || (iteration % 1000 == 0)) {
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
        bitset<64> curr_bits = prev_bits ^ xor_val;
        double curr_value = bitset_to_double(curr_bits);
        
        if(iteration < 20 || (iteration % 1000 == 0)) {
            cout << "  Decoded value: " << curr_value << endl;
        }
        
        decoded_values.push_back(curr_value);
        prev_bits = curr_bits;
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