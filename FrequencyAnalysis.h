#ifndef FREQUENCY_ANALYSIS_H
#define FREQUENCY_ANALYSIS_H

#include <fftw3.h>
#include <utility> // For std::pair

std::pair<double, double> findFundamentalFrequency(fftw_complex* output, int N, int samplerate);
double refineFrequency(int peakIndex, double* magnitudes, int N, double sampleRate);

#endif // FREQUENCY_ANALYSIS_H