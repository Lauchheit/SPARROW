// decode.cpp
#include "lz4_compression.h"
#include <iostream>
#include <cstring>

std::vector<double> LZ4Compression::decode(const BinaryFileReader& reader) {
    std::cout << "\n---------- LZ4 DECODE ----------" << std::endl;
    
    std::vector<bool> bits = reader.getSignalCode();
    
    // Read N (first 64 bits)
    uint64_t N = 0;
    for(int i = 0; i < 64; i++) {
        N = (N << 1) | (bits[i] ? 1 : 0);
    }
    
    std::cout << "Expected samples: " << N << std::endl;
    
    // Convert remaining bits to bytes
    std::vector<char> compressed;
    for(size_t i = 64; i < bits.size(); i += 8) {
        unsigned char byte = 0;
        for(int j = 0; j < 8 && (i + j) < bits.size(); j++) {
            byte = (byte << 1) | (bits[i + j] ? 1 : 0);
        }
        compressed.push_back(byte);
    }
    
    // Decompress
    int decompressed_size = N * sizeof(double);
    std::vector<char> decompressed(decompressed_size);
    
    int result = LZ4_decompress_safe(compressed.data(), decompressed.data(),
                                     compressed.size(), decompressed_size);
    
    if (result < 0) {
        throw std::runtime_error("LZ4 decompression failed");
    }
    
    // Convert bytes back to doubles
    std::vector<double> signal(N);
    std::memcpy(signal.data(), decompressed.data(), decompressed_size);
    
    std::cout << "Decoded " << N << " samples" << std::endl;
    
    return signal;
}