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
    if (m_music)        { Mix_HaltMusic(); Mix_FreeMusic(m_music); m_music = nullptr; }
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
    playMusicFrom(path, 0.0, 800, loops);
}

void AudioSystem::playMusicFrom(const std::string& path, double startPos, int fadeMs, int loops) {
    if (!m_ready) return;
    Mix_HaltMusic();
    if (m_music) { Mix_FreeMusic(m_music); m_music = nullptr; }
    m_currentPath = path;
    m_music = Mix_LoadMUS(path.c_str());
    if (!m_music) {
        std::cerr << "Mix_LoadMUS(" << path << ") failed: " << Mix_GetError() << "\n";
        m_currentPath.clear();
        return;
    }
    if (startPos > 0.0)
        Mix_FadeInMusicPos(m_music, loops, fadeMs, startPos);
    else
        Mix_FadeInMusic(m_music, loops, fadeMs);
}

void AudioSystem::stopMusic() {
    if (!m_ready) return;
    Mix_FadeOutMusic(600);
    m_currentPath.clear();
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

// Shot: punchy sawtooth chirp with noise crunch layer — 850 Hz → 60 Hz over 110ms
Mix_Chunk* AudioSystem::synthShot() {
    const int   SR      = 44100;
    const float DUR     = 0.11f;
    const float F_START = 850.f;
    const float F_END   = 60.f;
    int n = (int)(SR * DUR);
    std::vector<Sint16> buf(n);

    srand(7331);
    static float phase = 0.f;
    for (int i = 0; i < n; ++i) {
        float frac = (float)i / n;
        float freq = F_START + (F_END - F_START) * frac * frac; // quadratic sweep down
        phase += freq / SR;
        if (phase > 1.f) phase -= 1.f;
        float saw  = 2.f * phase - 1.f;

        // 20% noise layer for crunch
        float noise = ((float)rand() / RAND_MAX) * 2.f - 1.f;
        float sig   = saw * 0.80f + noise * 0.20f;

        // Sharp initial punch: slow the decay slightly at the start
        float amp = std::pow(1.f - frac, 1.3f) * 0.75f;
        buf[i] = (Sint16)(sig * amp * 30000.f);
    }
    return buildChunk(buf, SR);
}

// Explosion: deep thud — low-pass filtered noise + strong sub-bass, long decay
Mix_Chunk* AudioSystem::synthExplosion() {
    const int   SR       = 44100;
    const float DUR      = 0.65f;   // longer = more sustain
    const float BASS_HZ  = 42.f;    // deep sub-bass frequency
    const float BASS_STR = 0.65f;   // strong sub-bass punch
    int n = (int)(SR * DUR);
    std::vector<Sint16> buf(n);

    srand(42);
    float prev = 0.f;
    for (int i = 0; i < n; ++i) {
        float frac  = (float)i / n;
        float noise = ((float)rand() / RAND_MAX) * 2.f - 1.f;

        // Heavier low-pass (α = 0.5) for more body
        prev = 0.5f * noise + 0.5f * prev;

        // Slower exponential decay — more sustained thud
        float amp = std::exp(-frac * 4.5f);

        // Deep sub-bass sine for physical impact
        float sub = std::sin(2.f * 3.14159f * BASS_HZ * (float)i / SR) * BASS_STR;

        // Brief noise burst at very start (first 3ms) for transient click
        float click = (i < (int)(SR * 0.003f)) ? 0.4f * noise : 0.f;

        buf[i] = (Sint16)((prev + sub + click) * amp * 32000.f);
    }
    return buildChunk(buf, SR);
}

// Threshold alarm: two-tone rising sweep with punchy noise attack
Mix_Chunk* AudioSystem::synthThreshold() {
    const int   SR  = 44100;
    const float DUR = 0.22f;
    int n = (int)(SR * DUR);
    std::vector<Sint16> buf(n);

    srand(99);
    for (int i = 0; i < n; ++i) {
        float t    = (float)i / SR;
        float frac = (float)i / n;

        // Rising sweep: 330 Hz → 1050 Hz (lower start = bassier alarm)
        float freq = 330.f + 720.f * frac;
        float wave = std::sin(2.f * 3.14159f * freq * t);

        // Short noise transient at start for attack punch (first 8ms)
        float noise   = ((float)rand() / RAND_MAX) * 2.f - 1.f;
        float noiseMix = (i < (int)(SR * 0.008f)) ? 0.5f * noise : 0.f;

        // Amplitude: full, then fade out over last 25%
        float amp  = (frac < 0.75f) ? 0.6f : 0.6f * (1.f - (frac - 0.75f) / 0.25f);

        buf[i] = (Sint16)((wave + noiseMix) * amp * 30000.f);
    }
    return buildChunk(buf, SR);
}
