// zstd_compression.h
#ifndef ZSTD_COMPRESSION_H
#define ZSTD_COMPRESSION_H

#include "../compression_algorithm.h"
#include <zstd.h>
#include <vector>

class ZstdCompression : public CompressionAlgorithm {
public:
    std::vector<bool> encode(const SignalContext& context) override;
    std::vector<double> decode(const BinaryFileReader& reader) override;
};

#endif