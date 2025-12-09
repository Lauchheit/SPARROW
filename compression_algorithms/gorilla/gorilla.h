#ifndef GORILLA_H
#define GORILLA_H

#include "../compression_algorithm.h"
#include "../../input_strategies/source_context.h"

class GorillaCompression : public CompressionAlgorithm {
public:
    std::vector<bool> encode(const std::string&) override;
    std::vector<double> decode(const BinaryFileReader&) override;
};

#endif // GORILLA_H