// zlib_compression.h
#ifndef ZLIB_COMPRESSION_H
#define ZLIB_COMPRESSION_H

#include "../compression_algorithm.h"
#include <zlib.h>
#include <vector>

class ZlibCompression : public CompressionAlgorithm {
public:
    std::vector<bool> encode(const std::string&) override;
    std::vector<double> decode(const BinaryFileReader&) override;
};

#endif