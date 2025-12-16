// zstd_compression.h
#ifndef GORILLAELF_COMPRESSION_H
#define GORILLAELF_COMPRESSION_H

#include "../compression_algorithm.h"
#include "../../helpers/bit_operations.h"
#include "../sparrow/sparrow_helpers.h"
#include "../gorilla/gorilla.h"
#include <zstd.h>

class GorillaElfCompression : public CompressionAlgorithm {
public:
    std::vector<bool> encode(const std::string&) override;
    std::vector<double> decode(const BinaryFileReader&) override;
};

#endif