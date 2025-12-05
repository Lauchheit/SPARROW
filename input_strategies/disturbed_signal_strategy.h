#ifndef DISTURBED_SIGNAL_STRATEGY_H
#define DISTURBED_SIGNAL_STRATEGY_H

#include "signal_source_strategy.h"
#include "clean_signal_strategy.h"
#include <cmath>
#include <random>

class DisturbedSignalStrategy : public SignalSourceStrategy {
private:
    int numPoints;
    int amplitude;
    double stdDev;
    
    static std::random_device rd;
    static std::mt19937 generator;
    
    
    double gaussRandom(double s) const {
        std::normal_distribution<double> distr(0, s);
        return distr(generator);
    }
    
    double fs(int x, int A, double S, int N) const {
        return CleanSignalStrategy::f(x, A, N) + (S) * sin(x);
    }
    
public:
    DisturbedSignalStrategy(int n, int a, double s) 
        : numPoints(n), amplitude(a), stdDev(s) {}
    
    std::vector<double> getSignal() const override {
        std::vector<double> signal;
        signal.reserve(numPoints);
        
        for (int i = 0; i < numPoints; i++) {
            signal.push_back(fs(i, amplitude, stdDev, numPoints));
        }
        
        return signal;
    }
    
    std::string getDescription() const override {
        return "Disturbed sinusoidal signal (N=" + std::to_string(numPoints) + 
               ", A=" + std::to_string(amplitude) + 
               ", σ=" + std::to_string(stdDev) + ")";
    }
};

std::random_device DisturbedSignalStrategy::rd;
std::mt19937 DisturbedSignalStrategy::generator(DisturbedSignalStrategy::rd());

#endif