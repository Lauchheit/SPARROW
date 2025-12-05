#ifndef SIGNAL_SOURCE_STRATEGY_H
#define SIGNAL_SOURCE_STRATEGY_H

#include <vector>
#include <string>

class SignalSourceStrategy {
public:
    virtual ~SignalSourceStrategy() = default;
    
    virtual std::vector<double> getSignal() const = 0;

    virtual std::string getDescription() const = 0;
};

#endif