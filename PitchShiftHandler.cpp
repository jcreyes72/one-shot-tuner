#include "PitchShiftHandler.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <sndfile.h>
#include "signalsmith-stretch.h"
#include <fftw3.h>

// Function to pitch shift the processed audio data using block-by-block (streaming) processing.
bool pitchShiftData(const std::vector<float>& AudioData,
                    int sampleRate,
                    int channels,
                    double semitoneShift,
                    const std::string& outputFile) {
    if (AudioData.empty()) {
        std::cerr << "Error: No audio data to process.\n";
        return false;
    }

    // 1. Initialize the Signalsmith stretcher.
    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    // You can experiment with presetCheaper if desired.
    // stretcher.presetCheaper(channels, sampleRate);
    stretcher.presetDefault(channels, sampleRate);

    // 2. Set the transpose semitones (with a tonality limit, e.g. 8000/sampleRate).
    stretcher.setTransposeSemitones(semitoneShift, 8000.0 / sampleRate);

    // 3. Calculate the DSP block size and pad input to a multiple of the block size.
    int blockSize = stretcher.blockSamples();  // typically set by presetDefault
    size_t channelSamples = AudioData.size() / channels;
    size_t numBlocks = (channelSamples + blockSize - 1) / blockSize; // ceiling division
    size_t paddedSamples = numBlocks * blockSize;

    // 4. Create de-interleaved input buffers for each channel and pad with zeros.
    std::vector<std::vector<float>> inputBuffers(channels, std::vector<float>(paddedSamples, 0.0f));
    for (size_t ch = 0; ch < static_cast<size_t>(channels); ++ch) {
        for (size_t i = 0; i < channelSamples; ++i) {
            inputBuffers[ch][i] = AudioData[i * channels + ch];
        }
    }

    // 5. Process the input in blocks (streaming) to maintain overlap-add continuity.
    //    We accumulate processed output per channel.
    std::vector<std::vector<float>> finalOutput(channels, std::vector<float>());
    int pos = 0;
    while (pos < static_cast<int>(paddedSamples)) {
        int currentBlock = std::min(blockSize, static_cast<int>(paddedSamples - pos)); // current block length

        // Temporary output buffers for this block.
        std::vector<std::vector<float>> blockOut(channels, std::vector<float>(currentBlock, 0.0f));
        // Build arrays of pointers for the current block.
        std::vector<float*> inPtrs(channels), outPtrs(channels);
        for (int ch = 0; ch < channels; ++ch) {
            inPtrs[ch]  = inputBuffers[ch].data() + pos;
            outPtrs[ch] = blockOut[ch].data();
        }

        // Process this block.
        stretcher.process(inPtrs.data(), currentBlock,
                          outPtrs.data(), currentBlock);

        // Append the processed block to the final output per channel.
        for (int ch = 0; ch < channels; ++ch) {
            finalOutput[ch].insert(finalOutput[ch].end(), blockOut[ch].begin(), blockOut[ch].end());
        }

        pos += currentBlock;
    }

    // 6. Flush any remaining DSP data.
    int flushSamples = stretcher.outputLatency();
    std::vector<std::vector<float>> flushOut(channels, std::vector<float>(flushSamples, 0.0f));
    std::vector<float*> flushPtrs(channels);
    for (int ch = 0; ch < channels; ++ch) {
        flushPtrs[ch] = flushOut[ch].data();
    }
    stretcher.flush(flushPtrs.data(), flushSamples);
    for (int ch = 0; ch < channels; ++ch) {
        finalOutput[ch].insert(finalOutput[ch].end(), flushOut[ch].begin(), flushOut[ch].end());
    }

    // 7. Determine an effective trim offset using a stronger energy criterion.
    int baseLatency = stretcher.outputLatency();
    int effectiveTrim = baseLatency;
    const int windowSize = 256;             // Adjust as needed
    const float rmsThreshold = 1e-4f;         // Adjust based on your signal level
    const int requiredWindows = 3;            // Number of consecutive windows required
    size_t totalSamples = finalOutput[0].size();
    int consecutiveHits = 0;

    for (size_t i = baseLatency; i < totalSamples - windowSize; i++) {
        float sumSq = 0.f;
        // Sum energy across all channels in this window.
        for (int j = 0; j < windowSize; j++) {
            for (int ch = 0; ch < channels; ch++) {
                float val = finalOutput[ch][i + j];
                sumSq += val * val;
            }
        }
        float rms = std::sqrt(sumSq / (windowSize * channels));
        if (rms > rmsThreshold) {
            consecutiveHits++;
            // If we've found the required number of consecutive windows, determine a trim point.
            if (consecutiveHits >= requiredWindows) {
                // Back off slightly so that we include the first window in the consecutive series.
                effectiveTrim = static_cast<int>(i) - ((requiredWindows - 1) * windowSize / 2);
                if (effectiveTrim < baseLatency)
                    effectiveTrim = baseLatency;
                break;
            }
        } else {
            consecutiveHits = 0;
        }
    }

    // 8. Trim the output by removing the pre-roll blank space.
    // Instead of forcing the output length to equal the original input (channelSamples),
    // we preserve everything from the detected start until the end. This will ensure that
    // the audio starts as early as possible.
    size_t outputFrames = finalOutput[0].size() - effectiveTrim;
    std::vector<std::vector<float>> trimmedOutput(channels, std::vector<float>());
    for (int ch = 0; ch < channels; ++ch) {
        trimmedOutput[ch] = std::vector<float>(
            finalOutput[ch].begin() + effectiveTrim, 
            finalOutput[ch].end()
        );
    }

    // 9. Interleave the trimmed output from each channel.
    std::vector<float> interleavedOutput(outputFrames * channels);
    for (size_t i = 0; i < outputFrames; i++) {
        for (int ch = 0; ch < channels; ch++) {
            interleavedOutput[i * channels + ch] = trimmedOutput[ch][i];
        }
    }

    // 10. Write the processed audio to the output file using libsndfile.
    SF_INFO sfinfoOut = {};
    sfinfoOut.samplerate = sampleRate;
    sfinfoOut.channels = channels;
    sfinfoOut.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &sfinfoOut);
    if (!outFile) {
        std::cerr << "Error: Unable to open output file: " << outputFile << "\n";
        return false;
    }
    sf_count_t framesWritten = sf_writef_float(outFile, interleavedOutput.data(), outputFrames);
    sf_close(outFile);
    if (framesWritten != static_cast<sf_count_t>(outputFrames)) {
        std::cerr << "Warning: Not all frames were written to the output file.\n";
    } else {
        std::cout << "Tuned file saved as: " << outputFile << "\n";
    }

    return true;
}
