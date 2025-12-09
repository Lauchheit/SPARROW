#include "gorilla.h"

#include <iostream>
#include <vector>
#include <bitset>
#include <algorithm>
#include <cmath>

#include "../../helpers/bit_operations.h"
#include "../../helpers/file_operations.h"
#include "../../input_strategies/source_context.h"

using namespace std;

// Helper function to get leading zeros in a bitset
int get_leading_zeros_gorilla(bitset<64> bits) {
    unsigned long long val = bits.to_ullong();
    if(val == 0) return 64;
    return __builtin_clzll(val);
}

// Helper function to get trailing zeros in a bitset
int get_trailing_zeros_gorilla(bitset<64> bits) {
    unsigned long long val = bits.to_ullong();
    if(val == 0) return 64;
    return __builtin_ctzll(val);
}

// Get the meaningful window (first 1 to last 1)
struct MeaningfulWindow {
    int leading_zeros;
    int trailing_zeros;
    int meaningful_bits; // Number of bits between first and last 1
    
    int start_pos() const { return leading_zeros; }
    int end_pos() const { return 64 - trailing_zeros; }
};

MeaningfulWindow get_meaningful_window(bitset<64> xor_val) {
    MeaningfulWindow window;
    window.leading_zeros = get_leading_zeros_gorilla(xor_val);
    window.trailing_zeros = get_trailing_zeros_gorilla(xor_val);
    
    if(window.leading_zeros == 64) {
        window.meaningful_bits = 0;
    } else {
        window.meaningful_bits = 64 - window.leading_zeros - window.trailing_zeros;
    }
    
    return window;
}

// Check if window_new fits within window_prev
bool window_fits_in_previous(const MeaningfulWindow& window_new, const MeaningfulWindow& window_prev) {
    return (window_new.leading_zeros >= window_prev.leading_zeros) && 
           (window_new.trailing_zeros >= window_prev.trailing_zeros);
}

// Extract meaningful bits from a bitset given a window
vector<bool> extract_meaningful_bits(bitset<64> bits, const MeaningfulWindow& window) {
    vector<bool> result;
    
    if(window.meaningful_bits == 0) {
        return result; // Empty vector for all-zero case
    }
    
    // Extract bits from position [leading_zeros] to [64 - trailing_zeros - 1]
    for(int i = 63 - window.leading_zeros; i >= window.trailing_zeros; i--) {
        result.push_back(bits[i]);
    }
    
    return result;
}

// Encode window information: 5 bits for leading zeros, 6 bits for meaningful length
vector<bool> encode_window_info(const MeaningfulWindow& window) {
    vector<bool> result;

    // Validate window
    int sum = window.leading_zeros + window.meaningful_bits + window.trailing_zeros;
    if(sum != 64) {
        cerr << "FATAL: Invalid window! lz=" << window.leading_zeros 
             << " mb=" << window.meaningful_bits 
             << " tz=" << window.trailing_zeros 
             << " sum=" << sum << " (expected 64)" << endl;
    }
    
    int lz = window.leading_zeros;
    int mb = window.meaningful_bits;
    
    // Check if leading_zeros exceeds 5-bit capacity
    if(lz > 31) {
        cerr << "WARNING: leading_zeros=" << lz << " > 31, clamping to 31" << endl;
        lz = 31;
    }
    
    if(mb > 0) {
        mb = mb - 1;
    }
    
    if(mb > 63) {
        cerr << "ERROR: meaningful_bits-1=" << mb << " > 63 after adjustment!" << endl;
        mb = 63;
    }
    
    cout << "  Encoding window: lz=" << window.leading_zeros 
         << " mb=" << window.meaningful_bits 
         << " (stored as " << mb << ")"
         << " tz=" << window.trailing_zeros << endl;
    
    // Encode leading zeros (5 bits)
    for(int i = 4; i >= 0; i--) {
        result.push_back((lz >> i) & 1);
    }
    
    // Encode meaningful bits length (6 bits, storing mb-1)
    for(int i = 5; i >= 0; i--) {
        result.push_back((mb >> i) & 1);
    }
    
    return result;
}

std::vector<bool> GorillaCompression::encode(const std::string& input_filepath) {
    cout << endl << "---------- GORILLA ENCODE ----------" << endl;
    
    SignalContext context(std::make_unique<FileSignalStrategy>(input_filepath));
    std::vector<double> values = context.getSignal();
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
    
    // Encode first value (64 bits)
    vector<bool> first_value_bits = double_to_bitvector(values[0]);
    output.insert(output.end(), first_value_bits.begin(), first_value_bits.end());
    
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
        bitset<64> prev_bits = double_to_bits(values[i-1]);
        bitset<64> curr_bits = double_to_bits(values[i]);
        bitset<64> xor_val = prev_bits ^ curr_bits;
        
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