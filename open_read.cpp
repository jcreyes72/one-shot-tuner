#include <iostream>
#include <aubio/aubio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audio_file>" << std::endl;
        return 1;
    }

    const char* filepath = argv[1];
    const uint_t samplerate = 44100; // Adjust if necessary
    const uint_t hop_size = 512;    // Size of each processing block

    // Initialize aubio note detection
    aubio_notes_t* notes = new_aubio_notes("default", 2048, hop_size, samplerate);

    // Open the audio file
    aubio_source_t* source = new_aubio_source(filepath, samplerate, hop_size);
    if (!source) {
        std::cerr << "Error: Unable to open audio file " << filepath << std::endl;
        del_aubio_notes(notes);
        return 1;
    }

    fvec_t* input = new_fvec(hop_size); // Input buffer
    fvec_t* pitch_output = new_fvec(1); // Output buffer for note detection

    std::cout << "Detected notes:" << std::endl;
    while (true) {
        uint_t frames_read = 0;
        aubio_source_do(source, input, &frames_read);

        if (frames_read == 0) break; // End of file

        aubio_notes_do(notes, input, pitch_output);
        float detected_note = fvec_get_sample(pitch_output, 0);

        if (detected_note > 0) {
            std::cout << "Note detected: MIDI " << detected_note << std::endl;
        }
        else {
            std::cout << "nothin" << std::endl;
        }
    }

    // Cleanup
    del_aubio_notes(notes);
    del_aubio_source(source);
    del_fvec(input);
    del_fvec(pitch_output);

    return 0;
}
