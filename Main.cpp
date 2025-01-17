#include <iostream>
#include <string>
#include "AudioFileHandler.h"
#include "FrequencyAnalysis.h"
#include "NoteMapping.h"

int main(int argc, char* argv[]) {
    // Check for correct usage
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audio_file>" << std::endl;
        return 1;
    }

    const char* filepath = argv[1];
    auto fileInfo = readAudioFile(filepath);

    // Check if the audio file was opened successfully
    if (!fileInfo.audioFile) {
        std::cerr << "Error: Unable to open audio file." << std::endl;
        return 1;
    }

    // Print audio file information
    std::cout << "Opened file: " << filepath << "\n";
    std::cout << "Sample rate: " << fileInfo.samplerate << " Hz\n";
    std::cout << "Channels: " << fileInfo.channels << "\n";
    std::cout << "Frames: " << fileInfo.frames << "\n";

    // Process audio file
    processAudio(fileInfo);

    // Cleanup
    sf_close(fileInfo.audioFile);
    return 0;
}
