// zstd_compression.h
#ifndef SPARROWELF_COMPRESSION_H
#define SPARROWELF_COMPRESSION_H

#include "../compression_algorithm.h"
#include "../../helpers/bit_operations.h"
#include "../sparrow/sparrow_helpers.h"
#include <zstd.h>

class SparrowElfCompression : public CompressionAlgorithm {
public:
    std::vector<bool> encode(const std::string&) override;
    std::vector<double> decode(const BinaryFileReader&) override;
};

#endif