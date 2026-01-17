#ifndef FREQUENCY_SELECTION_H
#define FREQUENCY_SELECTION_H

#include <vector>
#include "../../global_parameters.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
struct FrequencyComponent {
    double frequency;
    double amplitude;
    double phase;
};
std::vector<FrequencyComponent> selectOptimalFrequencies(
    const std::vector<double>& signal, 
    double sampleRate
);

std::vector<double> reconstructSignal(
    const std::vector<FrequencyComponent>& frequencies,
    int N,
    double sampleRate
);


#endif // FREQUENCY_SELECTION_H