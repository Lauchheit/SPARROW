#include "zlib.h"
#include <iostream>
#include <cstring>

std::vector<double> ZlibCompression::decode(const BinaryFileReader& reader) {
    std::cout << "\n---------- ZLIB DECODE ----------" << std::endl;
    
    std::vector<bool> bits = reader.getSignalCode();
    
    // Read N (first 64 bits)
    uint64_t N = 0;
    for(int i = 0; i < 64; i++) {
        N = (N << 1) | (bits[i] ? 1 : 0);
    }
    
    std::cout << "Expected samples: " << N << std::endl;
    
    // Convert remaining bits to bytes
    std::vector<uint8_t> compressed;
    for(size_t i = 64; i < bits.size(); i += 8) {
        uint8_t byte = 0;
        for(int j = 0; j < 8 && (i + j) < bits.size(); j++) {
            byte = (byte << 1) | (bits[i + j] ? 1 : 0);
        }
        compressed.push_back(byte);
    }
    
    // Decompress
    uLongf decompressed_size = N * sizeof(double);
    std::vector<uint8_t> decompressed(decompressed_size);
    
    int result = uncompress(decompressed.data(), &decompressed_size,
                           compressed.data(), compressed.size());
    
    if (result != Z_OK) {
        throw std::runtime_error("Zlib decompression failed");
    }
    
    // Convert bytes back to doubles
    std::vector<double> signal(N);
    std::memcpy(signal.data(), decompressed.data(), decompressed_size);
    
    std::cout << "Decoded " << N << " samples" << std::endl;
    
    return signal;
}