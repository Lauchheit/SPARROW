#include "frequency_selection.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fftw3.h>

#include "../../global_parameters.h"



struct Spectrum {
    std::vector<double> amplitudes;
    std::vector<double> phases;
    std::vector<double> frequencies;
};

static Spectrum computeSpectrum(const std::vector<double>& xs, double sampleRate) {
    int N = xs.size();
    
    double* in = (double*) fftw_malloc(sizeof(double) * N);
    fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N/2 + 1));
    
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
    
    for(int i = 0; i < N; i++) {
        in[i] = xs[i];
    }
    
    fftw_execute(plan);
    
    Spectrum result;
    int numFreqs = N/2 + 1;
    
    for(int i = 0; i < numFreqs; i++) {
        double real = out[i][0];
        double imag = out[i][1];
        
        double amplitude = sqrt(real*real + imag*imag) / N;
        if(i > 0 && i < N/2) {
            amplitude *= 2.0;
        }
        result.amplitudes.push_back(amplitude);
        
        double phase = atan2(imag, real);
        result.phases.push_back(phase);
        
        double frequency = i * sampleRate / N;
        result.frequencies.push_back(frequency);
    }
    
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    
    return result;
}

std::vector<FrequencyComponent> selectOptimalFrequencies(
    const std::vector<double>& signal, 
    double sampleRate
) {
    int N = signal.size();
    
    Spectrum spectrum = computeSpectrum(signal, sampleRate);

    std::vector<size_t> indices(spectrum.amplitudes.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), 
              [&](size_t a, size_t b) { 
                  return spectrum.amplitudes[a] > spectrum.amplitudes[b]; 
              });
    
    std::vector<FrequencyComponent> selected_frequencies;
    double cost_per_frequency = 3 * GlobalParams::PRECISION;
    
    double epsilon = 0;
    for(const auto& xi : signal) {
        epsilon += std::abs(xi);
    }
    epsilon /= N;
    double max_amplitude = spectrum.amplitudes[indices[0]];
    for(size_t idx : indices) {
        double Ai = spectrum.amplitudes[idx];
        
        if(Ai < GlobalParams::FREQUENCY_THRESHOLD) break;
        if(Ai < max_amplitude/GlobalParams::RELATIVE_FREQUENCY_THRESHHOLD) break;
        if(selected_frequencies.size() > pow(2,GlobalParams::BITS_FOR_SAVED_FREQUENCIES)-1) break; //case: frequencies would exceed uint16 size
        if(selected_frequencies.size() > 100) break;
        double epsilon_new = epsilon - (2.0 * Ai / M_PI);
        
        if(epsilon_new <= 0) {
            selected_frequencies.push_back({
                spectrum.frequencies[idx],
                spectrum.amplitudes[idx],
                spectrum.phases[idx]
            });
            epsilon = 0.0001;
            continue;
        }
        
        double delta_LZ = std::log2(epsilon / epsilon_new);
        double benefit = N * delta_LZ;
        
        if(benefit > cost_per_frequency) {
            selected_frequencies.push_back({
                spectrum.frequencies[idx],
                spectrum.amplitudes[idx],
                spectrum.phases[idx]
            });
            epsilon = epsilon_new;
        }
    }
    
    
    return selected_frequencies;
}

std::vector<double> reconstructSignal(
    const std::vector<FrequencyComponent>& frequencies,
    int N,
    double sampleRate
) {
    std::vector<double> signal(N, 0.0);
    
    for(const auto& freq : frequencies) {
        for(int i = 0; i < N; i++) {
            double t = i / sampleRate;
            signal[i] += freq.amplitude * std::cos(2 * M_PI * freq.frequency * t + freq.phase);
        }
    }
    
    return signal;
}