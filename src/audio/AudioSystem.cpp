#include "AudioSystem.hpp"
#include <cmath>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <iostream>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

AudioSystem::~AudioSystem() {
    if (m_sndShot)      { Mix_FreeChunk(m_sndShot);      m_sndShot      = nullptr; }
    if (m_sndExplosion) { Mix_FreeChunk(m_sndExplosion); m_sndExplosion = nullptr; }
    if (m_sndThreshold) { Mix_FreeChunk(m_sndThreshold); m_sndThreshold = nullptr; }
    if (m_music)        { Mix_FreeMusic(m_music);         m_music        = nullptr; }
    if (m_ready) {
        Mix_CloseAudio();
        Mix_Quit();
    }
}

bool AudioSystem::init() {
    if (Mix_Init(MIX_INIT_MP3) == 0) {
        std::cerr << "Mix_Init (MP3) failed: " << Mix_GetError() << "\n";
        // Non-fatal: continue without MP3 support
    }

    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) < 0) {
        std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << "\n";
        return false;
    }

    Mix_AllocateChannels(8);
    m_ready = true;

    m_sndShot      = synthShot();
    m_sndExplosion = synthExplosion();
    m_sndThreshold = synthThreshold();

    return true;
}

// ---------------------------------------------------------------------------
// Music
// ---------------------------------------------------------------------------

void AudioSystem::playMusic(const std::string& path, int loops) {
    if (!m_ready) return;
    if (m_music) { Mix_FreeMusic(m_music); m_music = nullptr; }
    m_music = Mix_LoadMUS(path.c_str());
    if (!m_music) {
        std::cerr << "Mix_LoadMUS(" << path << ") failed: " << Mix_GetError() << "\n";
        return;
    }
    Mix_PlayMusic(m_music, loops);
}

void AudioSystem::stopMusic() {
    if (!m_ready) return;
    Mix_HaltMusic();
    if (m_music) { Mix_FreeMusic(m_music); m_music = nullptr; }
}

void AudioSystem::setMusicVolume(int vol) {
    if (m_ready) Mix_VolumeMusic(vol);
}

// ---------------------------------------------------------------------------
// SFX
// ---------------------------------------------------------------------------

void AudioSystem::playShot() {
    if (!m_ready || !m_sndShot) return;
    Mix_VolumeChunk(m_sndShot, m_sfxVol);
    Mix_PlayChannel(-1, m_sndShot, 0);
}

void AudioSystem::playExplosion() {
    if (!m_ready || !m_sndExplosion) return;
    Mix_VolumeChunk(m_sndExplosion, m_sfxVol);
    Mix_PlayChannel(-1, m_sndExplosion, 0);
}

void AudioSystem::playThreshold() {
    if (!m_ready || !m_sndThreshold) return;
    Mix_VolumeChunk(m_sndThreshold, m_sfxVol);
    Mix_PlayChannel(-1, m_sndThreshold, 0);
}

void AudioSystem::setSfxVolume(int vol) {
    m_sfxVol = vol;
}

// ---------------------------------------------------------------------------
// PCM synthesis
// ---------------------------------------------------------------------------

// Build a Mix_Chunk from a mono sample buffer; SDL_mixer works in stereo
// so we interleave to stereo and handle the WAV header manually.
Mix_Chunk* AudioSystem::buildChunk(const std::vector<Sint16>& mono, int sampleRate) {
    // Convert mono → stereo
    int nSamples = (int)mono.size();
    int byteLen  = nSamples * 2 * sizeof(Sint16); // stereo

    // Allocate Mix_Chunk with raw PCM buffer (no WAV header — Mix_QuickLoad_RAW)
    Uint8* buf = (Uint8*)SDL_malloc(byteLen);
    if (!buf) return nullptr;
    Sint16* dst = reinterpret_cast<Sint16*>(buf);
    for (int i = 0; i < nSamples; ++i) {
        dst[i * 2 + 0] = mono[i]; // L
        dst[i * 2 + 1] = mono[i]; // R
    }

    Mix_Chunk* chunk = Mix_QuickLoad_RAW(buf, byteLen);
    if (!chunk) { SDL_free(buf); }
    return chunk;
}

// Shot: frequency-swept sawtooth chirp, 600 Hz → 120 Hz over 90ms
Mix_Chunk* AudioSystem::synthShot() {
    const int   SR     = 44100;
    const float DUR    = 0.09f;
    const float F_START = 600.f;
    const float F_END   = 120.f;
    int n = (int)(SR * DUR);
    std::vector<Sint16> buf(n);
    for (int i = 0; i < n; ++i) {
        float t    = (float)i / SR;
        float frac = (float)i / n;
        float freq = F_START + (F_END - F_START) * frac * frac; // quadratic sweep
        // sawtooth phase
        static float phase = 0.f;
        phase += freq / SR;
        if (phase > 1.f) phase -= 1.f;
        float saw  = 2.f * phase - 1.f;
        float amp  = (1.f - frac) * 0.6f; // linear decay
        buf[i] = (Sint16)(saw * amp * 28000.f);
    }
    return buildChunk(buf, SR);
}

// Explosion: white noise with exponential decay over 0.45s, low-pass approximated
// by averaging adjacent samples for a "thud" character
Mix_Chunk* AudioSystem::synthExplosion() {
    const int   SR  = 44100;
    const float DUR = 0.45f;
    int n = (int)(SR * DUR);
    std::vector<Sint16> buf(n);
    // Generate raw noise first
    srand(42); // deterministic
    float prev = 0.f;
    for (int i = 0; i < n; ++i) {
        float frac  = (float)i / n;
        float noise = ((float)rand() / RAND_MAX) * 2.f - 1.f;
        // Simple one-pole low-pass for body (α = 0.4)
        prev = 0.4f * noise + 0.6f * prev;
        float amp = std::exp(-frac * 7.f); // fast exponential decay
        // Pitch down effect: mix in sub-bass sine
        float sub = std::sin(2.f * 3.14159f * 60.f * (float)i / SR) * 0.4f;
        buf[i] = (Sint16)((prev + sub) * amp * 28000.f);
    }
    return buildChunk(buf, SR);
}

// Threshold alarm: rising two-tone pulse (100ms)
Mix_Chunk* AudioSystem::synthThreshold() {
    const int   SR  = 44100;
    const float DUR = 0.18f;
    int n = (int)(SR * DUR);
    std::vector<Sint16> buf(n);
    for (int i = 0; i < n; ++i) {
        float t    = (float)i / SR;
        float frac = (float)i / n;
        float f1   = 440.f + 660.f * frac; // rising sweep
        float wave = std::sin(2.f * 3.14159f * f1 * t);
        float amp  = (frac < 0.8f) ? 0.5f : 0.5f * (1.f - (frac - 0.8f) / 0.2f);
        buf[i] = (Sint16)(wave * amp * 28000.f);
    }
    return buildChunk(buf, SR);
}
