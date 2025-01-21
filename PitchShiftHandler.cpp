#include "PitchShiftHandler.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <rubberband/RubberBandStretcher.h>
#include <sndfile.h>

bool pitchShiftData(const std::vector<float>& audioData,
                    int sampleRate,
                    int channels,
                    double semitoneShift,
                    const std::string& outputFile) {
    // 1. Calculate pitch factor
    double pitchFactor = std::pow(2.0, semitoneShift / 12.0);

    // 2. Initialize RubberBand
    RubberBand::RubberBandStretcher stretcher(sampleRate, channels,
        RubberBand::RubberBandStretcher::OptionProcessRealTime);

    stretcher.setTimeRatio(1.0); // Maintain original length
    stretcher.setPitchScale(pitchFactor);

    // 3. De-interleave audio data into separate channel buffers
    std::vector<std::vector<float>> channelBuffers(channels, std::vector<float>(audioData.size() / channels));
    for (size_t i = 0; i < audioData.size(); ++i) {
        int channel = i % channels;
        channelBuffers[channel][i / channels] = audioData[i];
    }

    // Create an array of pointers to channel data
    std::vector<const float*> channelPointers(channels);
    for (int ch = 0; ch < channels; ++ch) {
        channelPointers[ch] = channelBuffers[ch].data();
    }

    // 4. Feed samples into Rubber Band
    stretcher.process(channelPointers.data(), audioData.size() / channels, false);

    // 5. Retrieve processed data
    int avail = stretcher.available();
    std::vector<float> outData(avail * channels);
    std::vector<std::vector<float>> outChannelBuffers(channels, std::vector<float>(avail));
    std::vector<float*> outChannelPointers(channels);
    for (int ch = 0; ch < channels; ++ch) {
        outChannelPointers[ch] = outChannelBuffers[ch].data();
    }

    int got = stretcher.retrieve(outChannelPointers.data(), avail);

    // Re-interleave output data
    for (int frame = 0; frame < got; ++frame) {
        for (int ch = 0; ch < channels; ++ch) {
            outData[frame * channels + ch] = outChannelBuffers[ch][frame];
        }
    }

    // 6. Write the processed samples to the output file
    SF_INFO sfinfoOut;
    sfinfoOut.samplerate = sampleRate;
    sfinfoOut.channels = channels;
    sfinfoOut.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &sfinfoOut);
    if (!outFile) {
        std::cerr << "Error: Unable to open output file: " << outputFile << std::endl;
        return false;
    }

    sf_writef_float(outFile, outData.data(), got);
    sf_close(outFile);

    // Example warning for missing data
    if (got != avail) {
        std::cerr << "Warning: Not all data was retrieved from the stretcher." << std::endl;
    }

    return true;
}
