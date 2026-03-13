#pragma once
#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <vector>

class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem();

    // Call once after SDL_Init
    bool init();

    // Music (streamed — one track at a time)
    void playMusic(const std::string& path, int loops = -1);
    void playMusicFrom(const std::string& path, double startPos, int fadeMs = 800, int loops = -1);
    void stopMusic();  // fades out over 600ms
    void setMusicVolume(int vol); // 0–128
    // Returns true if the given track is currently streaming
    bool isPlaying(const std::string& path) const { return Mix_PlayingMusic() && m_currentPath == path; }

    // One-shot synthesised SFX (pre-generated at init)
    void playShot();
    void playExplosion();
    void playThreshold(); // trace threshold crossed
    void setSfxVolume(int vol); // 0–128

    bool ready() const { return m_ready; }

private:
    bool        m_ready       = false;
    Mix_Music*  m_music       = nullptr;
    std::string m_currentPath;
    int         m_sfxVol      = 80;

    Mix_Chunk* m_sndShot      = nullptr;
    Mix_Chunk* m_sndExplosion = nullptr;
    Mix_Chunk* m_sndThreshold = nullptr;

    // PCM synthesis helpers
    static Mix_Chunk* synthShot();
    static Mix_Chunk* synthExplosion();
    static Mix_Chunk* synthThreshold();
    static Mix_Chunk* buildChunk(const std::vector<Sint16>& samples, int sampleRate);
};
