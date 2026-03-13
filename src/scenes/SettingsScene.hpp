#pragma once
#include "IScene.hpp"

class SettingsScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    enum class Row { DisplayMode = 0, Resolution, Music, SFX, KeybindsInfo, Back, COUNT };

    Row   m_cursor      = Row::DisplayMode;
    float m_time        = 0.f;
    int   m_musicVol    = 80;
    int   m_sfxVol      = 80;
    int   m_displayMode = 0;   // 0=windowed, 1=fullscreen, 2=borderless
    int   m_resIdx      = 0;   // index into RESOLUTIONS[]

    void applyVolumes(SceneContext& ctx) const;
    void applyDisplay(SceneContext& ctx) const;
    void save(SceneContext& ctx) const;
    void goBack(SceneContext& ctx);
    void changeValue(int delta, SceneContext& ctx);
};
