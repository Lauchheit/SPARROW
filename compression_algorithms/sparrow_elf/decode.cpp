#include "sparrowelf_compression.h"

#include <iostream>
#include <vector>
#include <bitset>
#include <cmath>

#include "elf.h"

#include "../../global_parameters.h"
#include "../../input_strategies/binary_reader.h"
#include "../../helpers/bit_operations.h"
#include "../sparrow/frequency_selection.h"

using namespace std;


double elf_reconstruct(double v_prime, int alpha) {
    // Formula from the paper: v = LeaveOut(v', α) + 10^(-α)
    return LeaveOut(v_prime, alpha) + std::pow(10, -alpha) * (v_prime < 0 ? -1 : 1);
}

double elf_reconstruct_roundup_exact(double v_prime, int alpha) {
    int64_t scale = 1;
    for (int i = 0; i < alpha; i++) scale *= 10;

    double scaled = v_prime * scale;

    int64_t rounded;
    if (v_prime >= 0.0) {
        rounded = (int64_t)std::ceil(scaled - 1e-12);
    } else {
        rounded = (int64_t)std::floor(scaled + 1e-12);
    }

    return (double)rounded / scale;
}


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

uint8_t beta_star_bits_to_uint8(const std::vector<bool>& bitvec) {
    if(bitvec.size() != GlobalParams::BETA_STAR_BITS_SIZE) {
        throw std::invalid_argument("bitvector must have exactly " + 
                                    std::to_string(GlobalParams::BETA_STAR_BITS_SIZE) + 
                                    " bits for beta_star");
    }
    
    uint8_t result = 0;
    
    for(int i = 0; i < GlobalParams::BETA_STAR_BITS_SIZE; i++) {
        result <<= 1;
        if(bitvec[i]) {
            result |= 1;
        }
    }
    
    return result;
}

int8_t calculate_sp(const double v_prime) {
    
    // Handle special cases
    if (v_prime == 0.0 || !std::isfinite(v_prime)) {
        return 0;  // or throw an exception
    }
    
    // SP(v) = floor(log10(|v|))
    // This gives the exponent in scientific notation
    // e.g., 123.456 = 1.23456 × 10^2, so SP = 2
    //       0.00456 = 4.56 × 10^-3, so SP = -3
    int sp = (int)std::floor(std::log10(std::abs(v_prime)));
    
    return sp;
}

uint16_t calculate_alpha(uint8_t beta_star, int sp) {
    // Formula from the paper: α = β - (SP(v) + 1)
    int alpha = beta_star - (sp + 1);
    
    // Ensure alpha is non-negative (shouldn't happen with valid data)
    if (alpha < 0) {
        return 0;
    }
    
    return static_cast<uint8_t>(alpha);
}

std::vector<double> SparrowElfCompression::decode(const BinaryFileReader& reader) {

    cout << endl << "---------- SPARROW ELF DECODE ----------" << endl;

    string check_filepath = "C:\\Users\\cleme\\OneDrive\\uni\\Informatik\\Bachelorarbeit\\code\\SPARROW\\data\\signal_data.txt";
    SignalContext context(std::make_unique<FileSignalStrategy>(check_filepath));
    std::vector<double> check = context.getSignal();

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

    vector<bool> n_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::BITS_FOR_N_DATA_POINTS);
    pos += GlobalParams::BITS_FOR_N_DATA_POINTS;
    uint64_t N = bitvector_to_uint64(n_bits);
    cout << "Number of samples (N): " << N << endl;

    int prefix_length = ceil(log2(max(1, (int)best_wl)));
    cout << "Prefix length: " << prefix_length << endl;

    vector<bitset<64>> reconstructed_residuals_bitset;

    cout << "Bits size: " << bits.size() << endl;

    cout << "-----------------------------------\nFirst bits of data section: ";
    for(int k = 0; k < 20; k++) {
        cout << bits[pos + k];
    }
    cout << endl;

    
    vector<bool> elf_control_bits;
    vector<int> leading_zeros_list;
    vector<int> significand_length_list;
    vector<bool> is_zero_flags;
    vector<uint8_t> beta_stars;

    // Read data values until EOF
    size_t idx = 0;
    for(uint64_t idx = 0; idx < N; idx++) {
        cout << endl << "idx1:" << idx << "." << endl;

        cout << endl << "Bits remaining: " << bits.size()-pos << endl;

        if(pos + 8 >= bits.size()) {
            break;  // No more samples
        }
        
        bool is_zero = !bits[pos];
        cout << " zc " << is_zero << endl;
        pos +=1;
        is_zero_flags.push_back(is_zero);
        if(is_zero){
            elf_control_bits.push_back(0);
            leading_zeros_list.push_back(0);
            significand_length_list.push_back(0);
            beta_stars.push_back(0);
            reconstructed_residuals_bitset.push_back(bitset<64>(0)); 
            continue;
        }

        bool elf_control_bit = bits[pos];
        pos += 1;
        elf_control_bits.push_back(elf_control_bit);
        cout << " ec " << elf_control_bit << " eb* ";

        uint8_t beta_star = 0;
        if(elf_control_bit){
            vector<bool> beta_star_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::BETA_STAR_BITS_SIZE);
            pos += GlobalParams::BETA_STAR_BITS_SIZE;
            print_bitvector(beta_star_bits);

            beta_star = beta_star_bits_to_uint8(beta_star_bits);
        }
        beta_stars.push_back(beta_star);

        // Read Control Bit
        bool sparrow_control_bit = bits[pos];
        pos += 1;
        cout << " sc " << sparrow_control_bit << endl << " sp ";

        int leading_zeros = -1;
        int significand_length = -1;
        bitset<64> decoded_residual;
        if(sparrow_control_bit){
            leading_zeros = best_wl;

            // Read erasure position (6 bits)
            vector<bool> erasure_pos_bits(bits.begin() + pos, bits.begin() + pos + 6);
            pos += 6;
            cout << " eep "; print_bitvector(erasure_pos_bits);

            significand_length = bitvector_to_erasure_pos(erasure_pos_bits);
            
            cout << " (erasure_pos=" << significand_length << ")" << endl;

            // Read the significand bits
            vector<bool> significand_bits(bits.begin() + pos, bits.begin() + pos + significand_length);
            pos += significand_length;
            cout << " d "; print_bitvector(significand_bits); cout << endl;

            size_t trailing_zeros_length = 64 - (leading_zeros + significand_length);

            // Reconstruct full 64-bit residual
            vector<bool> reconstructed_residual_bits(64, false);

            // Fill only the significand part
            for(int i = 0; i < significand_length; i++) {
                reconstructed_residual_bits[leading_zeros + i] = significand_bits[i];
            }

            cout << endl << "Reconstruction: "; 
            print_bitvector(reconstructed_residual_bits);
            cout << endl<<endl;

            // Convert to bitset<64>
            decoded_residual = bitvector_to_bitset64(reconstructed_residual_bits);

        } else {
            // Case 2: LZ < w_l
            vector<bool> leading_zeros_bits(bits.begin() + pos, bits.begin() + pos + prefix_length);
            pos += prefix_length;
            print_bitvector(leading_zeros_bits);

            leading_zeros = 0;
            for(int j = 0; j < prefix_length; j++) {
                leading_zeros <<= 1;
                if(leading_zeros_bits[j]) {
                    leading_zeros |= 1;
                }
            }
            
            // Read erasure position
            vector<bool> erasure_pos_bits(bits.begin() + pos, bits.begin() + pos + 6);
            pos += 6;
            significand_length = bitvector_to_erasure_pos(erasure_pos_bits);

            cout << " eep "; print_bitvector(erasure_pos_bits);

            
            // Read significand
            vector<bool> significand_bits(bits.begin() + pos, bits.begin() + pos + significand_length);
            pos += significand_length;
            
            // Reconstruct
            vector<bool> reconstructed_residual_bits(64, false);
            for(int i = 0; i < significand_length; i++) {
                reconstructed_residual_bits[leading_zeros + i] = significand_bits[i];
            }
            
            decoded_residual = bitvector_to_bitset64(reconstructed_residual_bits);
        }
        leading_zeros_list.push_back(leading_zeros);
        significand_length_list.push_back(significand_length);
        reconstructed_residuals_bitset.push_back(decoded_residual);

    }

    cout << endl << "Decoded " << N << " residuals" << endl;
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

    vector<bitset<64>> elf_erased_bits;
    vector<double> xs_reconstructed;

    for(int i = 0; i < N; i++) {
        cout << endl << "idx2:" << i << "." << endl;

        if(is_zero_flags[i]) {
            xs_reconstructed.push_back(0); 
            continue;
        }
        // Perform XOR to get the original value back
        bitset<64> v_prime_bits = x_approx_bits[i] ^ reconstructed_residuals_bitset[i];
        elf_erased_bits.push_back(v_prime_bits);


        double v_prime = bitset_to_double(v_prime_bits);
        double v_original = v_prime;
        cout << "v': " << v_prime << endl << "Was erased: " << elf_control_bits[i] << endl;
        if(elf_control_bits[i]){

            cout << "Leading zeros: " << leading_zeros_list[i] << endl << "Significant Length: " << significand_length_list[i] << endl;

            cout << "v' before: " << v_prime_bits << endl;

            size_t erasure_position = leading_zeros_list[i] + significand_length_list[i];
            cout << "Erasing form position " << erasure_position << endl;
            for(int j = erasure_position; j < 64; j++) {
                v_prime_bits[63-j] = 0;
            }


            v_prime = bitset_to_double(v_prime_bits);
            
            cout << "v' after: " << v_prime_bits << endl << "As double: " << v_prime << endl;

            uint8_t beta_star = beta_stars[i];
            cout << "b*: " << (int)beta_star << endl;
            int8_t significand_position = calculate_sp(v_prime);
            cout << "SP(v): " << (int)significand_position << endl;
            uint16_t alpha = calculate_alpha(beta_star, significand_position);
            cout << "alpha: " << (int)alpha << endl;
            v_original = elf_reconstruct_roundup_exact(v_prime, alpha);
        }
        else{
            if(beta_stars[i]) cout << "ERROR: Beta Star is not 0 even though elf control bit is";
        }
        cout << endl<< "Reconstructed: " << v_original << endl;
        if(v_original != check[i]) {
            cout << "ERROR: Mismatching values: " << endl << "Original: " << check[i] << endl << "Reconstruction: " << v_original << endl;
            cout << "Reconstructed Residual: " << reconstructed_residuals_bitset[i] << endl << "Approximation: " << x_approx_bits[i] << endl << "v' (XOR): " << v_prime_bits << endl;
            cout << endl << "Reconstruction: "; print_bitvector(double_to_bitvector(v_original)); cout << endl;
            cout <<         "         Check: "; print_bitvector(double_to_bitvector(check[i])); cout << endl;
        }
        xs_reconstructed.push_back(v_original);

    }

    return xs_reconstructed;
}