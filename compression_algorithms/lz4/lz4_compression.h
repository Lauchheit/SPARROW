// lz4_compression.h
#ifndef LZ4_COMPRESSION_H
#define LZ4_COMPRESSION_H

#include "../compression_algorithm.h"
#include <lz4.h>

class LZ4Compression : public CompressionAlgorithm {
public:
    std::vector<bool> encode(const SignalContext& context) override;
    std::vector<double> decode(const BinaryFileReader& reader) override;
};

#endif