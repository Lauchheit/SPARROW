#include <iostream>
#include <vector>
#include <bitset>
#include <cmath>

#include "sparrowelf_compression.h"

#include "../../global_parameters.h"
#include "../../input_strategies/binary_reader.h"
#include "../../helpers/bit_operations.h"
#include "../sparrow/frequency_selection.h"
#include "elf.h"
using namespace std;


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
    }

    // Read w_l (16 bits)
    vector<bool> wl_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::BITS_FOR_WL);
    pos += GlobalParams::BITS_FOR_WL;
    uint16_t best_wl = bitvector_to_uint16(wl_bits);
    cout << "Window length (w_l): " << best_wl << endl;

    int prefix_length = ceil(log2(max(1, (int)best_wl)));
    cout << "Prefix length: " << prefix_length << endl;

    // Read ELF metadata and Sparrow residuals
    vector<bool> elf_flags;
    vector<int> beta_stars;
    vector<bitset<64>> r_bit;

    while(pos < bits.size()) {
        // Read ELF flag (1 bit)
        if(pos + 1 > bits.size()) break;
        
        bool elf_applied = bits[pos];
        pos += 1;
        elf_flags.push_back(elf_applied);
        
        // Read beta_star if ELF was applied (4 bits)
        int beta_star = 0;
        if(elf_applied) {
            if(pos + 4 > bits.size()) {
                cerr << "Warning: Not enough bits for beta_star" << endl;
                break;
            }
            for(int b = 0; b < 4; b++) {
                beta_star = (beta_star << 1) | bits[pos];
                pos++;
            }
        }
        beta_stars.push_back(beta_star);
        
        // Read Sparrow control bit (1 bit)
        if(pos + 1 > bits.size()) break;
        
        bool control_bit = bits[pos];
        pos += 1;

        bitset<64> decoded_residual;

        if(control_bit) {
            // Case 1: LZ >= w_l
            int significand_length = 64 - best_wl;
            
            if(pos + significand_length > bits.size()) {
                cerr << "Warning: Not enough bits for significand" << endl;
                break;
            }
            
            vector<bool> significand_bits(bits.begin() + pos, bits.begin() + pos + significand_length);
            pos += significand_length;

            // Reconstruct: w_l leading zeros + significand
            for(int j = 63; j >= 0; j--) {
                if(j >= significand_length) {
                    decoded_residual[j] = 0;
                } else {
                    decoded_residual[j] = significand_bits[significand_length - 1 - j];
                }
            }

        } else {
            // Case 2: LZ < w_l
            if(pos + prefix_length > bits.size()) {
                cerr << "Warning: Not enough bits for prefix" << endl;
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
            
            int significand_length = 64 - leading_zeros;

            if(pos + significand_length > bits.size()) {
                cerr << "Warning: Not enough bits for significand" << endl;
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

        r_bit.push_back(decoded_residual);
    }

    int N = r_bit.size();
    cout << "Decoded " << N << " residuals" << endl;

    // Reconstruct the approximated signal from frequencies
    vector<double> approximated_signal(N, 0.0);
    for(int i = 0; i < N; i++){
        for(const auto& freq : frequencies) {
            double t = (double)i / N;
            approximated_signal[i] += freq.amplitude * cos(2 * M_PI * freq.frequency * t + freq.phase);
        }
    }

    // Convert to bits for XOR
    vector<bitset<64>> x_approx_bits = vector_double_to_bits(approximated_signal);

    // XOR to get erased originals, then apply ELF restoration
    vector<double> xs_reconstructed;
    for(int i = 0; i < N; i++) {
        // XOR to get the erased original value
        bitset<64> xs_erased_bits = x_approx_bits[i] ^ r_bit[i];
        double xs_erased = bitset_to_double(xs_erased_bits);
        
        // Apply ELF restoration if it was applied
        double xs_original;
        if(elf_flags[i]) {
            if (xs_erased == 0.0) {
                // Handle zero case
                xs_original = 0.0;
            } else if(beta_stars[i] == 0) {
                // Special case: v = 10^(-i)
                int sp = (int)floor(log10(abs(xs_erased)));
                xs_original = pow(10, -(sp + 1));
            } else {
                // Normal case
                int sp = (int)floor(log10(abs(xs_erased)));
                int alpha = beta_stars[i] - (sp + 1);
                xs_original = LeaveOut(xs_erased, alpha) + pow(10, -alpha);
            }
        } else {
            xs_original = xs_erased;
        }
        
        xs_reconstructed.push_back(xs_original);
    }

    cout << "Reconstructed " << xs_reconstructed.size() << " values" << endl;

    return xs_reconstructed;
}