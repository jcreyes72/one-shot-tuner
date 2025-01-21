#ifndef PITCH_SHIFT_HANDLER_H
#define PITCH_SHIFT_HANDLER_H

#include <string>
#include <vector>

// Takes in:
//  - audioData: The mono audio data as a vector of floats
//  - sampleRate: The sample rate of the audio
//  - channels: Number of channels (should be 1 for mono data)
//  - semitoneShift: How many semitones to shift (+ up, - down) so the note becomes C
//  - outputFile: Destination path for the tuned file
//
// Returns whether the pitch shift succeeded.
bool pitchShiftData(const std::vector<float>& audioData,
                    int sampleRate,
                    int channels,
                    double semitoneShift,
                    const std::string& outputFile);

#endif // PITCH_SHIFT_HANDLER_H
