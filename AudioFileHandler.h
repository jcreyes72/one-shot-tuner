#ifndef AUDIO_FILE_HANDLER_H
#define AUDIO_FILE_HANDLER_H

#include <sndfile.h>
#include <vector>

struct AudioFileInfo {
    SNDFILE* audioFile;
    SF_INFO fileInfo;
    int samplerate;
    int channels;
    int frames;
};

AudioFileInfo readAudioFile(const char* filepath);
void processAudio(const AudioFileInfo& fileInfo);

#endif // AUDIO_FILE_HANDLER_H
