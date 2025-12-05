#ifndef ENCODER_H
#define ENCODER_H

#include <vector>
#include <string>
#include "../input_strategies/signal_source_strategy.h"
#include "../input_strategies/source_context.h"
#include "../input_strategies/binary_reader.h"

class SignalContext;
class BinaryFileReader;

class CompressionAlgorithm {
public:
    ~CompressionAlgorithm() = default;
    
    virtual std::vector<bool> encode(const SignalContext&);
    virtual std::vector<double> decode(const BinaryFileReader&);
};

#endif