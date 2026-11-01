#include "audio.hpp"
#include <iostream>
#include <algorithm>

void AmbientEngine::init(float sampleRate) {
    m_padVoices.resize(7);
    m_pluckVoices.resize(5);
    m_delay.init(static_cast<size_t>(sampleRate * 2.0f));

    triggerNextPadChord(sampleRate);
}

void AmbientEngine::triggerNextPadChord(float sampleRate) {
    const auto& chord = m_chords[m_currentChord];
    m_currentChord = (m_currentChord + 1) % m_chords.size();

    for (size_t i = 0; i < chord.size() && i < m_padVoices.size(); ++i) {
        float freq = midiToFreq(chord[i]);
        float pan = 0.2f + 0.6f * (static_cast<float>(i) / chord.size());
        m_padVoices[i].trigger(freq, 5.0f, 10.0f, sampleRate, 0.12f, pan);
    }
}

void AmbientEngine::triggerPluck(float sampleRate) {
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    
    if (prob(m_rng) > 0.40f) {
        for (auto& voice : m_pluckVoices) {
            if (!voice.env.isActive()) {
                std::uniform_int_distribution<size_t> noteDist(0, m_scale.size() - 1);
                int midi = m_scale[noteDist(m_rng)];
                
                std::uniform_real_distribution<float> panDist(0.1f, 0.9f);
                voice.trigger(midiToFreq(midi), 0.02f, 4.0f, sampleRate, 0.08f, panDist(m_rng));
                break;
            }
        }
    }
}

void AmbientEngine::update(float dt) {
    m_padTimer += dt;
    m_pluckTimer += dt;

    if (m_padTimer >= PAD_INTERVAL) {
        m_padTimer = 0.0f;
        triggerNextPadChord(44100.0f);
    }

    if (m_pluckTimer >= PLUCK_INTERVAL) {
        m_pluckTimer = 0.0f;
        triggerPluck(44100.0f);
    }
}

void AmbientEngine::render(float* outL, float* outR, size_t frames, float sampleRate) {
    size_t delaySamples = static_cast<size_t>(sampleRate * 0.45f);

    for (size_t i = 0; i < frames; ++i) {
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (auto& v : m_padVoices) {
            v.process(sampleRate, sampleL, sampleR);
        }

        for (auto& v : m_pluckVoices) {
            v.process(sampleRate, sampleL, sampleR);
        }

        m_delay.process(sampleL, sampleR, outL[i], outR[i], delaySamples);
    }
}

bool AudioManager::setup() {
    int err = snd_pcm_open(&m_soundWave, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) return false;

    err = snd_pcm_set_params(
        m_soundWave,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        CHANNELS,
        SAMPLE_RATE,
        1,
        50000
    );
    if (err < 0) return false;

    m_engine.init(static_cast<float>(SAMPLE_RATE));
    return true;
}

void AudioManager::update() {
    float dt = static_cast<float>(BUFFER_FRAMES) / static_cast<float>(SAMPLE_RATE);
    
    m_engine.update(dt);

    float left[BUFFER_FRAMES];
    float right[BUFFER_FRAMES];

    m_engine.render(left, right, BUFFER_FRAMES, static_cast<float>(SAMPLE_RATE));

    for (size_t i = 0; i < BUFFER_FRAMES; ++i) {
        float l = std::clamp(left[i], -1.0f, 1.0f);
        float r = std::clamp(right[i], -1.0f, 1.0f);

        m_buffer[i * 2 + 0] = static_cast<int16_t>(l * 32767.0f);
        m_buffer[i * 2 + 1] = static_cast<int16_t>(r * 32767.0f);
    }

    snd_pcm_sframes_t frames = snd_pcm_writei(m_soundWave, m_buffer, BUFFER_FRAMES);
    if (frames < 0) {
        frames = snd_pcm_recover(m_soundWave, frames, 0);
        if (frames < 0) {
            std::cerr << "ALSA write failed: " << snd_strerror(frames) << "\n";
        }
    }
}

void AudioManager::cleanup() {
    if (m_soundWave) {
        snd_pcm_drain(m_soundWave);
        snd_pcm_close(m_soundWave);
        m_soundWave = nullptr;
    }
}