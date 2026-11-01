#pragma once
#include <alsa/asoundlib.h>
#include <vector>
#include <cmath>
#include <cstdint>
#include <random>

inline float midiToFreq(int note) {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

struct Oscillator {
    float frequency = 440.0f;
    float phase = 0.0f;

    float process(float sampleRate) {
        float sample = std::sin(phase);
        phase += 2.0f * static_cast<float>(M_PI) * frequency / sampleRate;
        if (phase >= 2.0f * static_cast<float>(M_PI)) {
            phase -= 2.0f * static_cast<float>(M_PI);
        }
        return sample;
    }
};

struct Envelope {
    enum State { IDLE, ATTACK, RELEASE };
    State state = IDLE;
    float level = 0.0f;
    float attackRate = 0.0001f;
    float releaseRate = 0.0001f;

    void trigger(float attackSec, float releaseSec, float sampleRate) {
        state = ATTACK;
        attackRate = 1.0f / (attackSec * sampleRate);
        releaseRate = 1.0f / (releaseSec * sampleRate);
    }

    float process() {
        if (state == ATTACK) {
            level += attackRate;
            if (level >= 1.0f) {
                level = 1.0f;
                state = RELEASE;
            }
        } else if (state == RELEASE) {
            level -= releaseRate;
            if (level <= 0.0f) {
                level = 0.0f;
                state = IDLE;
            }
        }
        return level;
    }

    bool isActive() const { return state != IDLE; }
};

struct SimpleDelay {
    std::vector<float> bufferL;
    std::vector<float> bufferR;
    size_t writeIdx = 0;
    float feedback = 0.65f;
    float mix = 0.40f;

    void init(size_t maxFrames) {
        bufferL.assign(maxFrames, 0.0f);
        bufferR.assign(maxFrames, 0.0f);
    }

    void process(float inL, float inR, float& outL, float& outR, size_t delayFrames) {
        if (bufferL.empty()) { outL = inL; outR = inR; return; }

        size_t readIdx = (writeIdx + bufferL.size() - delayFrames) % bufferL.size();
        float delayedL = bufferL[readIdx];
        float delayedR = bufferR[readIdx];

        bufferL[writeIdx] = inL + delayedL * feedback;
        bufferR[writeIdx] = inR + delayedR * feedback;

        writeIdx = (writeIdx + 1) % bufferL.size();

        outL = inL * (1.0f - mix) + delayedL * mix;
        outR = inR * (1.0f - mix) + delayedR * mix;
    }
};

struct Voice {
    Oscillator osc;
    Envelope env;
    float pan = 0.5f;
    float gain = 0.2f;

    void trigger(float freq, float attack, float release, float sampleRate, float volume = 0.2f, float stereoPan = 0.5f) {
        osc.frequency = freq;
        gain = volume;
        pan = stereoPan;
        env.trigger(attack, release, sampleRate);
    }

    void process(float sampleRate, float& outL, float& outR) {
        if (!env.isActive()) return;
        float s = osc.process(sampleRate) * env.process() * gain;
        outL += s * (1.0f - pan);
        outR += s * pan;
    }
};

class AmbientEngine {
public:
    void init(float sampleRate);
    void update(float dt);
    void render(float* outL, float* outR, size_t frames, float sampleRate);

private:
    std::mt19937 m_rng{1337};
    std::vector<Voice> m_padVoices;
    std::vector<Voice> m_pluckVoices;
    SimpleDelay m_delay;

    float m_padTimer = 0.0f;
    float m_pluckTimer = 0.0f;
    const float PAD_INTERVAL = 17.0f;
    const float PLUCK_INTERVAL = 2.5f;

    std::vector<int> m_scale = {50, 52, 54, 56, 57, 59, 61, 62, 64, 66, 68, 69, 71, 73, 74};
    
    std::vector<std::vector<int>> m_chords = {
        {50, 57, 61}, // Dmaj7
        {52, 56, 61}, // E6/9
        {54, 57, 62}, // F#m
        {57, 61, 64}  // Aadd9
    };
    size_t m_currentChord = 0;

    void triggerNextPadChord(float sampleRate);
    void triggerPluck(float sampleRate);
};

class AudioManager {
public:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNELS = 2;
    static constexpr size_t BUFFER_FRAMES = 512;

    bool setup();
    void update();
    void cleanup();

private:
    snd_pcm_t* m_soundWave = nullptr;
    int16_t m_buffer[BUFFER_FRAMES * CHANNELS];
    AmbientEngine m_engine;
};