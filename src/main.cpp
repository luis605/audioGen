#include "audio.hpp"

int main() {
    AudioManager audio;
    if (!audio.setup()) {
        return 1;
    }

    while (true) {
        audio.update();
    }

    audio.cleanup();
    return 0;
}