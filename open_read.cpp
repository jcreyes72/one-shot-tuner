#include <iostream>
#include <cmath>
#include <fftw3.h>
#include <sndfile.h>
#include <string>

// Function to convert frequency to note
std::string frequencyToNote(double frequency) {
    if (frequency <= 0) return "No valid note";

    // Frequencies for one octave (C4 to B4)
    static const double noteFrequencies[] = {
        261.63, // C
        277.18, // C#
        293.66, // D
        311.13, // D#
        329.63, // E
        349.23, // F
        369.99, // F#
        392.00, // G
        415.30, // G#
        440.00, // A
        466.16, // A#
        493.88  // B
    };
    const std::string notes[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    // Normalize frequency to the range of C4 to B4
    while (frequency < noteFrequencies[0]) frequency *= 2; // Bring up to C4
    while (frequency >= noteFrequencies[11] * 2) frequency /= 2; // Bring down to B4

    // Find the closest note in the octave
    double minDifference = std::numeric_limits<double>::max();
    int closestNoteIndex = 0;

    for (int i = 0; i < 12; ++i) {
        double difference = std::abs(frequency - noteFrequencies[i]);
        if (difference < minDifference) {
            minDifference = difference;
            closestNoteIndex = i;
        }
    }

    return notes[closestNoteIndex];
}

// Main program remains the same...
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audio_file>" << std::endl;
        return 1;
    }

    const char* filepath = argv[1];
    SF_INFO fileInfo = {0};
    SNDFILE* audioFile = sf_open(filepath, SFM_READ, &fileInfo);

    if (!audioFile) {
        std::cerr << "Error: Unable to open audio file: " << sf_strerror(nullptr) << std::endl;
        return 1;
    }

    // Display file information
    std::cout << "Opened file: " << filepath << "\n";
    std::cout << "Sample rate: " << fileInfo.samplerate << " Hz\n";
    std::cout << "Channels: " << fileInfo.channels << "\n";
    std::cout << "Frames: " << fileInfo.frames << "\n";

    // FFT parameters
    const int N = 8192; // Larger FFT window size for better resolution
    std::vector<double> signal(N, 0.0); // Input signal buffer
    fftw_complex* output = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N / 2 + 1)); // Output frequency domain buffer
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, signal.data(), output, FFTW_ESTIMATE);

    std::vector<float> audioBuffer(N * fileInfo.channels); // Temporary audio buffer for file reading

    // Process the audio file in chunks
    while (sf_readf_float(audioFile, audioBuffer.data(), N / fileInfo.channels) > 0) {
        // Convert multichannel audio to mono by averaging channels
        for (int i = 0; i < N; ++i) {
            signal[i] = 0.0;
            for (int ch = 0; ch < fileInfo.channels; ++ch) {
                signal[i] += audioBuffer[i * fileInfo.channels + ch];
            }
            signal[i] /= fileInfo.channels; // Average channels
        }

        // Perform FFT
        fftw_execute(plan);

        // Find the fundamental frequency (lowest significant frequency)
        double fundamentalFrequency = 0.0;
        double maxMagnitude = 0.0;

        for (int i = 0; i < N / 2; ++i) {
            double real = output[i][0];
            double imag = output[i][1];
            double magnitude = sqrt(real * real + imag * imag);

            // Consider only frequencies with significant magnitudes
            if (magnitude > 0.01 && i * (fileInfo.samplerate / N) > 20) {
                if (magnitude > maxMagnitude) {
                    maxMagnitude = magnitude;
                    fundamentalFrequency = i * (double)fileInfo.samplerate / N;
                }
            }
        }

        std::string note = frequencyToNote(fundamentalFrequency);
        std::cout << "Fundamental frequency: " << fundamentalFrequency 
                  << " Hz, Magnitude: " << maxMagnitude 
                  << ", Note: " << note << "\n";
    }

    // Cleanup
    fftw_destroy_plan(plan);
    fftw_free(output);
    sf_close(audioFile);

    return 0;
}