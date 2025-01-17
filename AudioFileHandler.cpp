#include "AudioFileHandler.h"
#include "FrequencyAnalysis.h"
#include "NoteMapping.h"
#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <cmath>       // for std::sqrt
#include "GlobalData.h"

std::unordered_map<std::string, int> noteCounts;         // To store note occurrences
std::unordered_map<std::string, double> noteMagnitudes;  // To store cumulative magnitudes

AudioFileInfo readAudioFile(const char* filepath) {
    SF_INFO fileInfo = {0};
    SNDFILE* audioFile = sf_open(filepath, SFM_READ, &fileInfo);

    AudioFileInfo info = {
        audioFile,
        fileInfo,
        fileInfo.samplerate,
        fileInfo.channels,
        static_cast<int>(fileInfo.frames)
    };

    return info;
}

void processAudio(const AudioFileInfo& fileInfo) {
    // Set FFT size
    const int N = 8192;
    std::vector<double> signal(N, 0.0);
    std::vector<float> audioBuffer(N * fileInfo.channels);

    fftw_complex* output = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N / 2 + 1));
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, signal.data(), output, FFTW_ESTIMATE);

    // Read and process audio in chunks of size N
    while (sf_readf_float(fileInfo.audioFile, audioBuffer.data(), N / fileInfo.channels) > 0) {
        // Convert multi-channel data to mono
        for (int i = 0; i < N; ++i) {
            signal[i] = 0.0;
            for (int ch = 0; ch < fileInfo.channels; ++ch) {
                signal[i] += audioBuffer[i * fileInfo.channels + ch];
            }
            signal[i] /= fileInfo.channels;
        }

        // Execute the FFT
        fftw_execute(plan);

        // Unpack frequency and magnitude
        auto [fundamentalFrequency, maxMagnitude] = findFundamentalFrequency(output, N, fileInfo.samplerate);
        std::string note = frequencyToNoteBinarySearch(fundamentalFrequency);

        // Update maps
        noteCounts[note]++;
        noteMagnitudes[note] += maxMagnitude;

        std::cout << "Fundamental frequency: " << fundamentalFrequency
                  << " Hz, Magnitude: " << maxMagnitude
                  << ", Note: " << note << "\n";
    }

    // Determine the overall note based on both count and cumulative magnitude
    std::string dominantNote;
    double bestScore = 0.0;

    for (const auto& [note, count] : noteCounts) {
        double sumOfMagnitudes = noteMagnitudes[note];
        // Proposed formula: sumOfMagnitudes * sqrt(count)
        double score = sumOfMagnitudes * std::sqrt(static_cast<double>(count));

        if (score > bestScore) {
            bestScore = score;
            dominantNote = note;
        }
    }

    std::cout << "Overall note: " << dominantNote << ", Score: " << bestScore << "\n";

    fftw_destroy_plan(plan);
    fftw_free(output);
}
