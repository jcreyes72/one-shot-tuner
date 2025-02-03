#ifndef PITCH_SHIFT_HANDLER_H
#define PITCH_SHIFT_HANDLER_H

#include <string>
#include <vector>

// Takes in:
//  - audioData: The audio data as a vector of floats (either mono or multi-channel)
//  - sampleRate: The sample rate of the audio
//  - channels: Number of channels
//  - semitoneShift: Number of semitones to shift (positive for upward, negative for downward)
//  - outputFile: Destination path for the tuned file
//
// Returns whether the pitch shift succeeded.
bool pitchShiftData(const std::vector<float>& audioData,
                    int sampleRate,
                    int channels,
                    double semitoneShift,
                    const std::string& outputFile);

#endif // PITCH_SHIFT_HANDLER_H
