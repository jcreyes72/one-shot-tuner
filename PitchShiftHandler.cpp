#include "PitchShiftHandler.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <sndfile.h>
#include "signalsmith-stretch.h"

// Function to pitch shift the processed audio data
bool pitchShiftData(const std::vector<float>& AudioData,
                    int sampleRate,
                    int channels,
                    double semitoneShift,
                    const std::string& outputFile) {
    if (AudioData.empty()) {
        std::cerr << "Error: No audio data to process.\n";
        return false;
    }

    // 1. Initialize the Signalsmith stretcher
    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    stretcher.presetDefault(channels, sampleRate);

    // 2. Set the transpose semitones 
    stretcher.setTransposeSemitones(semitoneShift);

    // 3. Prepare non-interleaved input buffers (required by Signalsmith)
    std::vector<std::vector<float>> inputBuffers(channels, std::vector<float>(AudioData.size() / channels));

    // Convert interleaved input to non-interleaved format
    for (size_t sample = 0; sample < AudioData.size() / channels; ++sample) {
        for (int ch = 0; ch < channels; ++ch) {
            inputBuffers[ch][sample] = AudioData[sample * channels + ch]; 
        }
    }

    // 4. Prepare output buffers with the **same size as input** (since we're not stretching)
    std::vector<std::vector<float>> outputBuffers(channels, std::vector<float>(inputBuffers[0].size()));

    // 5. Process the audio without time-stretching
    stretcher.process(inputBuffers.data(), inputBuffers[0].size(), outputBuffers.data(), outputBuffers[0].size());


    // 6. Convert non-interleaved output back to interleaved format for libsndfile
    std::vector<float> interleavedOutput(outputBuffers[0].size() * channels);

    for (size_t sample = 0; sample < outputBuffers[0].size(); ++sample) {
        for (int ch = 0; ch < channels; ++ch) {
            interleavedOutput[sample * channels + ch] = outputBuffers[ch][sample];
        }
    }


    // 7. Set up libsndfile output configuration
    SF_INFO sfinfoOut = {};
    sfinfoOut.samplerate = sampleRate;
    sfinfoOut.channels = channels;
    sfinfoOut.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    // 8. Open the output file
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &sfinfoOut);
    if (!outFile) {
        std::cerr << "Error: Unable to open output file: " << outputFile << std::endl;
        return false;
    }

    // 9. Write the processed audio data to the output file
    sf_count_t framesWritten = sf_writef_float(outFile, interleavedOutput.data(), interleavedOutput.size() / channels);
    sf_close(outFile);

    if (framesWritten != static_cast<sf_count_t>(interleavedOutput.size() / channels)) {
        std::cerr << "Warning: Not all frames were written to the output file.\n";
    } else {
        std::cout << "Tuned file saved as: " << outputFile << "\n";
    }

    return true;

}
