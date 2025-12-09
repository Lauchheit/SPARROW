// encode.cpp
#include "zstd_compression.h"
#include <iostream>
#include <cstring>

std::vector<bool> ZstdCompression::encode(const std::string& input_filepath) {
    std::cout << "\n---------- ZSTD ENCODE ----------" << std::endl;
    
    SignalContext context(std::make_unique<FileSignalStrategy>(input_filepath));
    std::vector<double> signal = context.getSignal();
    size_t N = signal.size();
    
    const void* src = signal.data();
    size_t src_size = N * sizeof(double);
    
    size_t max_dst_size = ZSTD_compressBound(src_size);
    std::vector<uint8_t> compressed(max_dst_size);
    
    size_t compressed_size = ZSTD_compress(compressed.data(), max_dst_size,
                                           src, src_size, 3);  // compression level 3
    
    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error(std::string("Zstd compression failed: ") + 
                                ZSTD_getErrorName(compressed_size));
    }
    
    compressed.resize(compressed_size);
    
    std::cout << "Original: " << src_size << " bytes" << std::endl;
    std::cout << "Compressed: " << compressed_size << " bytes" << std::endl;
    std::cout << "Ratio: " << (double)compressed_size / src_size << std::endl;
    
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