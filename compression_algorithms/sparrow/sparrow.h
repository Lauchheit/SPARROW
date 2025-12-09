#ifndef MY_ALGORITHM_H
#define MY_ALGORITHM_H

#include "../compression_algorithm.h"
#include "../../input_strategies/source_context.h"

class SparrowCompression : public CompressionAlgorithm {
public:
    std::vector<bool> encode(const std::string&) override;
    std::vector<double> decode(const BinaryFileReader&) override;
};

#endif
