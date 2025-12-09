#include "zlib.h"
#include <iostream>
#include <cstring>
#include <vector>

std::vector<bool> ZlibCompression::encode(const SignalContext& context) {
    std::cout << "\n---------- ZLIB ENCODE ----------" << std::endl;
    
    std::vector<double> signal = context.getSignal();
    size_t N = signal.size();
    
    std::vector<uint8_t> input(N * sizeof(double));
    std::memcpy(input.data(), signal.data(), input.size());
    
    uLongf compressed_size = compressBound(input.size());
    std::vector<uint8_t> compressed(compressed_size);
    
    int result = compress(compressed.data(), &compressed_size,
                         input.data(), input.size());
    
    if (result != Z_OK) {
        throw std::runtime_error("Zlib compression failed");
    }
    
    compressed.resize(compressed_size);
    
    std::cout << "Original: " << input.size() << " bytes" << std::endl;
    std::cout << "Compressed: " << compressed_size << " bytes" << std::endl;
    std::cout << "Ratio: " << (double)compressed_size / input.size() << std::endl;
    
    std::vector<bool> bits;
    
    // Store N (64 bits)
    for(int i = 63; i >= 0; i--) {
        bits.push_back((N >> i) & 1);
    }
    
    // Store compressed data
    for(uint8_t byte : compressed) {
        for(int i = 7; i >= 0; i--) {
            bits.push_back((byte >> i) & 1);
        }
    }
    
    return bits;
}