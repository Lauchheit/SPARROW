#ifndef GORILLA_HELPERS_H
#define GORILLA_HELPERS_H

#include <bitset>
#include <vector>

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


// Get the meaningful window (first 1 to last 1)
struct MeaningfulWindow {
    int leading_zeros;
    int trailing_zeros;
    int meaningful_bits; // Number of bits between first and last 1
    
    int start_pos() const { return leading_zeros; }
    int end_pos() const { return 64 - trailing_zeros; }
};

// Decoding helper functions
DecodeMeaningfulWindow decode_window_info(const std::vector<bool>& bits, size_t& pos);
std::bitset<64> reconstruct_from_meaningful_bits(const std::vector<bool>& meaningful_bits, 
                                                  const DecodeMeaningfulWindow& window);

// Encoding helper functions
int get_leading_zeros_gorilla(std::bitset<64> bits);
int get_trailing_zeros_gorilla(std::bitset<64> bits);
MeaningfulWindow get_meaningful_window(std::bitset<64> xor_val);
bool window_fits_in_previous(const MeaningfulWindow& window_new, 
                             const MeaningfulWindow& window_prev);
std::vector<bool> extract_meaningful_bits(std::bitset<64> bits, 
                                          const MeaningfulWindow& window);
std::vector<bool> encode_window_info(const MeaningfulWindow& window);

#endif // GORILLA_HELPERS_H