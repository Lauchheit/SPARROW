#ifndef SIGNAL_CONTEXT_H
#define SIGNAL_CONTEXT_H

#include "signal_source_strategy.h"
#include <memory>
#include <vector>
#include <stdexcept>

class SignalContext {
private:
    std::unique_ptr<SignalSourceStrategy> strategy;
    
public:
    SignalContext(std::unique_ptr<SignalSourceStrategy> initialStrategy) 
        : strategy(std::move(initialStrategy)) {}
    
    void setStrategy(std::unique_ptr<SignalSourceStrategy> newStrategy) {
        strategy = std::move(newStrategy);
    }
    
    std::vector<double> getSignal() const {
        if (!strategy) {
            throw std::runtime_error("No strategy set");
        }
        return strategy->getSignal();
    }
    
    std::string getStrategyDescription() const {
        if (!strategy) {
            return "No strategy set";
        }
        return strategy->getDescription();
    }
};

#endif 