// encode.cpp
#include "lz4_compression.h"
#include <iostream>
#include <cstring>

std::vector<bool> LZ4Compression::encode(const SignalContext& context) {
    std::cout << "\n---------- LZ4 ENCODE ----------" << std::endl;
    
    std::vector<double> signal = context.getSignal();
    size_t N = signal.size();
    
    const char* src = reinterpret_cast<const char*>(signal.data());
    int src_size = N * sizeof(double);
    
    int max_dst_size = LZ4_compressBound(src_size);
    std::vector<char> compressed(max_dst_size);
    
    int compressed_size = LZ4_compress_default(src, compressed.data(), 
                                                src_size, max_dst_size);
    
    if (compressed_size <= 0) {
        throw std::runtime_error("LZ4 compression failed");
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
    for(char byte : compressed) {
        for(int i = 7; i >= 0; i--) {
            bits.push_back(((unsigned char)byte >> i) & 1);
        }
    }
    
    return bits;
}