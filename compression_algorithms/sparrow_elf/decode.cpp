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

    cout << endl << "---------- SPARROW ELF DECODE ----------" << endl;

    // Read binary
    vector<bool> bits = reader.getSignalCode();
    cout << "Total bits read: " << bits.size() << endl;

    size_t pos = 0;

    /* ------------------------------------------------------------
       Read saved frequencies
    ------------------------------------------------------------ */

    vector<bool> freq_count_bits(bits.begin() + pos, bits.begin() + pos + 16);
    pos += GlobalParams::BITS_FOR_SAVED_FREQUENCIES;

    uint16_t num_frequencies = bitvector_to_uint16(freq_count_bits);
    cout << "Number of frequencies: " << num_frequencies << endl;

    vector<FrequencyComponent> frequencies;
    for (int i = 0; i < num_frequencies; i++) {

        vector<bool> freq_bits(bits.begin() + pos, bits.begin() + pos + 64);
        pos += 64;
        double frequency = bitvector_to_double(freq_bits);

        vector<bool> ampl_bits(bits.begin() + pos, bits.begin() + pos + 64);
        pos += 64;
        double amplitude = bitvector_to_double(ampl_bits);

        vector<bool> phase_bits(bits.begin() + pos, bits.begin() + pos + 64);
        pos += 64;
        double phase = bitvector_to_double(phase_bits);

        frequencies.push_back({frequency, amplitude, phase});
    }

    /* ------------------------------------------------------------
       Read Sparrow parameters
    ------------------------------------------------------------ */

    vector<bool> wl_bits(bits.begin() + pos, bits.begin() + pos + GlobalParams::BITS_FOR_WL);
    pos += GlobalParams::BITS_FOR_WL;

    uint16_t best_wl = bitvector_to_uint16(wl_bits);
    cout << "Window length (w_l): " << best_wl << endl;

    int prefix_length = ceil(log2(max(1, (int)best_wl)));
    cout << "Prefix length: " << prefix_length << endl;

    /* ------------------------------------------------------------
       Decode residuals (ELF + Sparrow)
    ------------------------------------------------------------ */

    vector<bool> elf_flags;
    vector<int> beta_stars;
    vector<bitset<64>> residual_bits;

    while (pos < bits.size()) {

        /* ---------- ELF flag ---------- */

        bool elf_applied = bits[pos++];
        elf_flags.push_back(elf_applied);

        int beta_star = 0;
        if (elf_applied) {
            for (int b = 0; b < 4; b++) {
                beta_star = (beta_star << 1) | bits[pos++];
            }
        }
        beta_stars.push_back(beta_star);

        int bits_to_erase = 0;
        if (elf_applied) {
            bits_to_erase = beta_star;  // MUST match encoder logic
        }

        /* ---------- Sparrow control ---------- */

        bool sparrow_control = bits[pos++];

        int leading_zeros = 0;
        if (!sparrow_control) {
            vector<bool> prefix(bits.begin() + pos, bits.begin() + pos + prefix_length);
            pos += prefix_length;

            for (bool b : prefix) {
                leading_zeros = (leading_zeros << 1) | b;
            }
        }

        /* ---------- Read kept significand bits ---------- */

        int full_mantissa_bits = 52;
        int kept_bits = full_mantissa_bits - bits_to_erase;
        kept_bits = max(0, kept_bits);

        vector<bool> kept_significand(bits.begin() + pos,
                                      bits.begin() + pos + kept_bits);
        pos += kept_bits;

        /* ---------- Reconstruct full 64-bit residual ---------- */

        bitset<64> r;
        r.reset();

        int mantissa_start = 12;

        // Sparrow leading zeros
        for (int i = 0; i < leading_zeros; i++) {
            r[mantissa_start + i] = 0;
        }

        // Copy kept mantissa bits
        for (int i = 0; i < kept_bits; i++) {
            r[mantissa_start + leading_zeros + i] = kept_significand[i];
        }

        // Remaining erased mantissa bits stay zero

        residual_bits.push_back(r);
    }

    cout << "Decoded " << residual_bits.size() << " residuals" << endl;

    /* ------------------------------------------------------------
       Reconstruct approximated signal
    ------------------------------------------------------------ */

    int N = residual_bits.size();
    vector<double> approximated_signal(N, 0.0);

    for (int i = 0; i < N; i++) {
        for (const auto& f : frequencies) {
            double t = (double)i / N;
            approximated_signal[i] +=
                f.amplitude * cos(2 * M_PI * f.frequency * t + f.phase);
        }
    }

    vector<bitset<64>> approx_bits = vector_double_to_bits(approximated_signal);

    /* ------------------------------------------------------------
       XOR residual + ELF restoration
    ------------------------------------------------------------ */

    vector<double> reconstructed;

    for (int i = 0; i < N; i++) {

        bitset<64> erased_bits = approx_bits[i] ^ residual_bits[i];
        double xs_erased = bitset_to_double(erased_bits);

        double xs_original;

        if (elf_flags[i]) {
            if (xs_erased == 0.0) {
                xs_original = 0.0;
            } else if (beta_stars[i] == 0) {
                int sp = (int)floor(log10(abs(xs_erased)));
                xs_original = pow(10, -(sp + 1));
            } else {
                int sp = (int)floor(log10(abs(xs_erased)));
                int alpha = beta_stars[i] - (sp + 1);
                xs_original = LeaveOut(xs_erased, alpha) + pow(10, -alpha);
            }
        } else {
            xs_original = xs_erased;
        }

        reconstructed.push_back(xs_original);
    }

    cout << "Reconstructed " << reconstructed.size() << " values" << endl;

    return reconstructed;
}
