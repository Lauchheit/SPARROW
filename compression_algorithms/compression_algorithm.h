#ifndef COMPRESSION_ALGORITHM_H
#define COMPRESSION_ALGORITHM_H

#include <vector>
#include <string>
#include "../input_strategies/signal_source_strategy.h"
#include "../input_strategies/source_context.h"
#include "../input_strategies/binary_reader.h"
#include "../input_strategies/file_signal_strategy.h"

class SignalContext;
class BinaryFileReader;

class CompressionAlgorithm {
public:
    virtual ~CompressionAlgorithm() = default;
    
    virtual std::vector<bool> encode(const std::string&) = 0;
    virtual std::vector<double> decode(const BinaryFileReader&) = 0;
};

#endif