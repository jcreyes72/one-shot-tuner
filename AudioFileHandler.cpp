#include "AudioFileHandler.h"
#include "FrequencyAnalysis.h"
#include "NoteMapping.h"
#include "PitchShiftHandler.h" // Include pitch-shifting logic
#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <cmath>       // for std::sqrt, std::cos, M_PI
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

    // Allocate memory for FFT output
    fftw_complex* output = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N / 2 + 1));

    // Create an FFTW plan
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, signal.data(), output, FFTW_ESTIMATE);

    // **Store both mono and stereo data**
    std::vector<float> monoAudioData;
    std::vector<float> stereoAudioData; // NEW: Store original stereo audio

    while (sf_readf_float(fileInfo.audioFile, audioBuffer.data(), N / fileInfo.channels) > 0) {
        // 1. Convert multi-channel data to mono and store stereo
        for (int i = 0; i < N; ++i) {
            signal[i] = 0.0;
            for (int ch = 0; ch < fileInfo.channels; ++ch) {
                signal[i] += audioBuffer[i * fileInfo.channels + ch];
            }
            signal[i] /= fileInfo.channels;
        }

        // **Store mono data for pitch detection**
        monoAudioData.insert(monoAudioData.end(), signal.begin(), signal.end());

        // **Store original stereo data for pitch shifting**
        stereoAudioData.insert(stereoAudioData.end(), audioBuffer.begin(), audioBuffer.end());

        // 2. Apply a Hann window to reduce spectral leakage
        for (int i = 0; i < N; ++i) {
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

    // **Use stereo data for pitch shifting**
    if (dominantNote != "C") {
        static const std::unordered_map<std::string, int> noteToSemitoneShift = {
            {"C#", -1}, {"D", -2}, {"D#", -3}, {"E", -4}, {"F", -5},
            {"F#", -6}, {"G", +5}, {"G#", +4}, {"A", +3}, {"A#", +2}, {"B", +1}
        };

        // Strip octave number
        std::string noteWithoutOctave = dominantNote.substr(0, (dominantNote[1] == '#') ? 2 : 1);

        int semitoneShift = noteToSemitoneShift.at(noteWithoutOctave);

        std::string outputFile = "tuned_" + dominantNote + ".wav";
        if (pitchShiftData(stereoAudioData, fileInfo.samplerate, fileInfo.channels, semitoneShift, outputFile)) {
            std::cout << "Tuned file saved as: " << outputFile << "\n";
        } else {
            std::cerr << "Error: Failed to tune the audio file.\n";
        }
    } else {
        std::cout << "The file is already tuned to C. No changes made.\n";
    }

    // Clean up
    fftw_destroy_plan(plan);
    fftw_free(output);
}
