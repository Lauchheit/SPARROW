#ifndef CLEAN_SIGNAL_STRATEGY_H
#define CLEAN_SIGNAL_STRATEGY_H

#include "signal_source_strategy.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class CleanSignalStrategy : public SignalSourceStrategy {
private:
    int numPoints;
    int amplitude;
    
public:
    static double f(int x, int A, int N) {
        return A*0.6*std::cos(2*M_PI*5*x/N+0.5) + A*0.4*std::cos(2*M_PI*12*x/N-1);

    }

    CleanSignalStrategy(int n, int a) : numPoints(n), amplitude(a) {}
    
    std::vector<double> getSignal() const override {
        std::vector<double> signal;
        signal.reserve(numPoints);
        
        for (int i = 0; i < numPoints; i++) {
            signal.push_back(f(i, amplitude, numPoints));
        }
        
        return signal;
    }
    
    std::string getDescription() const override {
        return "Clean sinusoidal signal (N=" + std::to_string(numPoints) + 
               ", A=" + std::to_string(amplitude) + ")";
    }
};

#endif // CLEAN_SIGNAL_STRATEGY_H