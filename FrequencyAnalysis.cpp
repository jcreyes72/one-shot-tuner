#include "FrequencyAnalysis.h"
#include <cmath>
#include <vector>
#include <utility>

std::pair<double, double> findFundamentalFrequency(fftw_complex* output, int N, int samplerate) {
    std::vector<double> magnitudes(N / 2);
    double maxMagnitude = 0.0;
    int peakIndex = 0;

    for (int i = 0; i < N / 2; ++i) {
        double real = output[i][0];
        double imag = output[i][1];
        magnitudes[i] = sqrt(real * real + imag * imag);

        if (magnitudes[i] > maxMagnitude) {
            maxMagnitude = magnitudes[i];
            peakIndex = i;
        }
    }

    double frequency = refineFrequency(peakIndex, magnitudes.data(), N, samplerate);
    return {frequency, maxMagnitude}; // Return frequency and magnitude as a pair
}

double refineFrequency(int peakIndex, double* magnitudes, int N, double sampleRate) {
    if (peakIndex <= 0 || peakIndex >= N / 2 - 1) {
        return peakIndex * sampleRate / N; // Cannot interpolate, return FFT bin frequency
    }

    double alpha = magnitudes[peakIndex - 1];
    double beta = magnitudes[peakIndex];
    double gamma = magnitudes[peakIndex + 1];

    // Parabolic interpolation formula
    double peakAdjustment = 0.5 * (alpha - gamma) / (alpha - 2 * beta + gamma);
    return (peakIndex + peakAdjustment) * sampleRate / N;
}