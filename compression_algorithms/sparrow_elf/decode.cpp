#include "sparrowelf_compression.h"

#include <iostream>
#include <vector>
#include <bitset>
#include <cmath>

#include "../../global_parameters.h"
#include "../../input_strategies/binary_reader.h"
#include "../../helpers/bit_operations.h"
#include "../sparrow/frequency_selection.h"

using namespace std;

int bitvector_to_erasure_pos(const std::vector<bool>& bitvec) {
    if(bitvec.size() != 6) {
        throw std::invalid_argument("bitvector must have exactly 6 bits for erasure_pos");
    }
    
    int result = 0;
    
    for(int i = 0; i < 6; i++) {
        result <<= 1;
        if(bitvec[i]) {
            result |= 1;
        }
    }
    
    return result;
}

std::vector<double> SparrowElfCompression::decode(const BinaryFileReader& reader) {

    cout << endl << "---------- SPARROW DECODE ----------" << endl;

    // Read binary
    vector<bool> bits = reader.getSignalCode();
    cout << "Total bits read: " << bits.size() << endl;

    size_t pos = 0;

    // Read first 16 bits to get the number of saved frequencies
    vector<bool> freq_count_bits(bits.begin() + pos, bits.begin() + pos + 16);
    pos += GlobalParams::BITS_FOR_SAVED_FREQUENCIES;
    uint16_t num_frequencies = bitvector_to_uint16(freq_count_bits);
    cout << "Number of frequencies: " << num_frequencies << endl;

    // Read frequencies
    vector<FrequencyComponent> frequencies;
    for(int i = 0; i < num_frequencies; i++) {
        // Frequency (64 bits)
        vector<bool> freq_bits(bits.begin() + pos, bits.begin() + pos + 64);
        pos += GlobalParams::PRECISION;
        double frequency = bitvector_to_double(freq_bits);

        // Amplitude (64 bits)
        vector<bool> ampl_bits(bits.begin() + pos, bits.begin() + pos + 64);
        pos += GlobalParams::PRECISION;
        double amplitude = bitvector_to_double(ampl_bits);

        // Phase (64 bits)
        vector<bool> phase_bits(bits.begin() + pos, bits.begin() + pos + 64);
        pos += GlobalParams::PRECISION;
        double phase = bitvector_to_double(phase_bits);

        frequencies.push_back({frequency, amplitude, phase});
        
        //cout << "Frequency " << i << ": f=" << frequency << ", A=" << amplitude << ", φ=" << phase << endl;
    }

    // Read w_l (16 bits)
    vector<bool> wl_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::BITS_FOR_WL);
    pos += GlobalParams::BITS_FOR_WL;
    uint16_t best_wl = bitvector_to_uint16(wl_bits);
    cout << "Window length (w_l): " << best_wl << endl;

    int prefix_length = ceil(log2(max(1, (int)best_wl)));
    cout << "Prefix length: " << prefix_length << endl;

    vector<bitset<64>> r_bit;

    cout << "Bits size: " << bits.size() << endl;

    // Read data values until EOF
    while(pos < bits.size()) {

        if(pos + 1 > bits.size()) {
            break;  // No more samples
        }

        bool elf_control_bit = bits[pos];
        pos += 1;
        cout << " ec " << elf_control_bit << " eb* ";

        if(elf_control_bit){
            vector<bool> beta_star_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::BETA_STAR_BITS_SIZE);
            pos += GlobalParams::BETA_STAR_BITS_SIZE;
            print_bitvector(beta_star_bits);
        }


        // Read Control Bit
        bool sparrow_control_bit = bits[pos];
        pos += 1;
        cout << " sc " << sparrow_control_bit << endl << " sp ";

        size_t leading_zeros;
        size_t significand_length;
        bitset<64> decoded_residual;
        if(sparrow_control_bit){
            leading_zeros = best_wl;

            // Read erasure position (6 bits)
            vector<bool> erasure_pos_bits(bits.begin() + pos, bits.begin() + pos + 6);
            pos += 6;
            cout << " eep "; print_bitvector(erasure_pos_bits);

            int erasure_position = bitvector_to_erasure_pos(erasure_pos_bits);
            int significand_length = erasure_position;
            
            cout << " (erasure_pos=" << erasure_position << ")" << endl;

            // Read the significand bits
            vector<bool> significand_bits(bits.begin() + pos, bits.begin() + pos + significand_length);
            pos += significand_length;
            cout << " d "; print_bitvector(significand_bits);

            size_t trailing_zeros_length = 64 - (leading_zeros + significand_length);

            // Reconstruct full 64-bit residual
            vector<bool> reconstructed_residual_bits(64, false);

            // Fill only the significand part
            for(int i = 0; i < significand_length; i++) {
                reconstructed_residual_bits[leading_zeros + i] = significand_bits[i];
            }

            cout << endl << "Reconstruction: "; 
            print_bitvector(reconstructed_residual_bits);
            cout << endl;

            // Convert to bitset<64>
            decoded_residual = bitvector_to_bitset64(reconstructed_residual_bits);

        } else {
            // Case 2: LZ < w_l
            vector<bool> leading_zeros_bits(bits.begin() + pos, bits.begin() + pos + prefix_length);
            pos += prefix_length;
            print_bitvector(leading_zeros_bits);

            int leading_zeros = 0;
            for(int j = 0; j < prefix_length; j++) {
                leading_zeros <<= 1;
                if(leading_zeros_bits[j]) {
                    leading_zeros |= 1;
                }
            }
            
            // Read erasure position
            vector<bool> erasure_pos_bits(bits.begin() + pos, bits.begin() + pos + 6);
            pos += 6;
            int erasure_position = bitvector_to_erasure_pos(erasure_pos_bits);
            
            // Read significand
            vector<bool> significand_bits(bits.begin() + pos, bits.begin() + pos + erasure_position);
            pos += erasure_position;
            
            // Reconstruct
            vector<bool> reconstructed_residual_bits(64, false);
            for(int i = 0; i < erasure_position; i++) {
                reconstructed_residual_bits[leading_zeros + i] = significand_bits[i];
            }
            
            decoded_residual = bitvector_to_bitset64(reconstructed_residual_bits);
        }

        r_bit.push_back(decoded_residual);

/*
        if(sparrow_control_bit) {
            // Case 1: LZ >= w_l
            // Format: [1][significand (64-w_l bits)]
            
            int significand_length = 64 - best_wl;
            
            if(pos + significand_length > bits.size()) {
                cerr << "Warning: Not enough bits for significand at position " << pos << endl;
                break;
            }
            
            // Read Significand
            vector<bool> significand_bits(bits.begin() + pos, bits.begin() + pos + significand_length);
            pos += significand_length;
            print_bitvector(significand_bits);

            // Reconstruct: w_l leading_zeros + significand
            for(int j = 63; j >= 0; j--) {
                if(j >= significand_length) {
                    decoded_residual[j] = 0;
                } else {
                    decoded_residual[j] = significand_bits[significand_length - 1 - j];
                }
            }

        } else {
            // Case 2: LZ < w_l
            // Format: [0][prefix (prefix_length bits)][significand (64-LZ bits)]
            
            if(pos + prefix_length > bits.size()) {
                cerr << "Warning: Not enough bits for prefix at position " << pos << endl;
                break;
            }
            
            vector<bool> prefix_bits(bits.begin() + pos, bits.begin() + pos + prefix_length);
            pos += prefix_length;

            int leading_zeros = 0;
            for(int j = 0; j < prefix_length; j++) {
                leading_zeros <<= 1;
                if(prefix_bits[j]) {
                    leading_zeros |= 1;
                }
            }
            
            // Read significand
            int significand_length = 64 - leading_zeros;

            if(pos + significand_length > bits.size()) {
                if (bits.size() - pos) cerr << "Warning: Not enough bits for significand at position " << pos << " (need " << significand_length << ", have " << (bits.size() - pos) << ")" << endl;
                break;
            }
            
            vector<bool> significand_bits(bits.begin() + pos, bits.begin() + pos + significand_length);
            pos += significand_length;

            for(int j = 63; j >= 0; j--) {
                if(j >= significand_length) {
                    decoded_residual[j] = 0;
                } else {
                    decoded_residual[j] = significand_bits[significand_length - 1 - j];
                }
            }
        }
*/
    }

    // Only now do we know N and can start with constructing the approximate signal
    int N = r_bit.size();

    cout << "Decoded " << N << " residuals" << endl;
    cout << "Final position: " << pos << " / " << bits.size() << " bits" << endl;

    int remaining = bits.size() - pos;
    if(remaining > 7) { 
        cerr << "Warning: " << remaining << " bits remaining (should be < 8 for padding)" << endl;
    }

    // Reconstruct the clean signal from its frequencies 
    vector<double> approximated_signal(N, 0.0);

    for(int i = 0; i < N; i++){
        for(const auto& freq : frequencies) {
            double t = (double)i / N;
            approximated_signal[i] += freq.amplitude * cos(2 * M_PI * freq.frequency * t + freq.phase);
        }
    }

    //Convert approximate signal to bits to perform XOR
    vector<bitset<64>> x_approx_bits = vector_double_to_bits(approximated_signal);

    vector<bitset<64>> xs_reconstructed_bits;
    for(int i = 0; i < N; i++) {
        // Perform XOR to get the original value back
        bitset<64> original_bits = x_approx_bits[i] ^ r_bit[i];
        xs_reconstructed_bits.push_back(original_bits);
    }

    // Convert back to double
    vector<double> xs_reconstructed;
    for(const auto& bits : xs_reconstructed_bits) {
        double value = bitset_to_double(bits);
        xs_reconstructed.push_back(value);
    }

    //write_doublevector_to_file(xs_reconstructed);


    return xs_reconstructed;
}