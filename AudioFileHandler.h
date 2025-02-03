#ifndef AUDIO_FILE_HANDLER_H
#define AUDIO_FILE_HANDLER_H

#include <sndfile.h>
#include <string>

// Structure to hold audio file information.
struct AudioFileInfo {
    SNDFILE* audioFile;
    SF_INFO fileInfo;
    int samplerate;
    int channels;
    int frames;
    std::string filepath;  // Added so we can re-read the original file
};

// Function declarations
AudioFileInfo readAudioFile(const char* filepath);
void processAudio(const AudioFileInfo& fileInfo);
bool tuneAudioFile(const std::string &filepath, int semitoneShift);

#endif // AUDIO_FILE_HANDLER_H
