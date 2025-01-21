#include "AudioFileHandler.h"
#include "FrequencyAnalysis.h"
#include "NoteMapping.h"
#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <cmath>       // for std::sqrt, std::cos, M_PI
#include "GlobalData.h"
#include "PitchShiftHandler.h" // Include pitch-shifting logic

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

    // Store processed audio data for pitch shifting
    std::vector<float> monoAudioData;

    while (sf_readf_float(fileInfo.audioFile, audioBuffer.data(), N / fileInfo.channels) > 0) {
        // 1. Convert multi-channel data to mono
        for (int i = 0; i < N; ++i) {
            signal[i] = 0.0;
            for (int ch = 0; ch < fileInfo.channels; ++ch) {
                signal[i] += audioBuffer[i * fileInfo.channels + ch];
            }
            signal[i] /= fileInfo.channels;
        }

        // Append the current signal chunk to the mono audio data
        monoAudioData.insert(monoAudioData.end(), signal.begin(), signal.end());

        // 2. Apply a Hann window to reduce spectral leakage
        for (int i = 0; i < N; ++i) {
            // Hann window formula: 0.5 * (1 - cos(2 * pi * i / (N - 1)))
            double hannValue = 0.5 * (1.0 - std::cos((2.0 * M_PI * i) / (N - 1)));
            signal[i] *= hannValue;
        }

        // 3. Execute the FFT
        fftw_execute(plan);

        // 4. Unpack frequency and magnitude
        auto [fundamentalFrequency, maxMagnitude] = findFundamentalFrequency(output, N, fileInfo.samplerate);
        std::string note = frequencyToNoteBinarySearch(fundamentalFrequency);

        // 5. Update maps
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

    // If the dominant note is not "C", apply pitch shifting
    if (dominantNote != "C") {
        static const std::unordered_map<std::string, int> noteToSemitoneShift = {
            {"C", 0}, {"C#", -1}, {"D", -2}, {"D#", -3}, {"E", -4}, {"F", -5},
            {"F#", -6}, {"G", -7}, {"G#", -8}, {"A", -9}, {"A#", -10}, {"B", -11}
        };

        int semitoneShift = noteToSemitoneShift.at(dominantNote);

        std::string outputFile = "tuned_" + dominantNote + ".wav";
        if (pitchShiftData(monoAudioData, fileInfo.samplerate, 1, semitoneShift, outputFile)) {
            std::cout << "Tuned file saved as: " << outputFile << "\n";
        } else {
            std::cerr << "Error: Failed to tune the audio file.\n";
        }
    } else {
        std::cout << "The file is already tuned to C. No changes made.\n";
    }

    fftw_destroy_plan(plan);
    fftw_free(output);
}