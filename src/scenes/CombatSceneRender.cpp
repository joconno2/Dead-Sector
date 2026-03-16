// CombatSceneRender.cpp — render, ICE/mine drawing, pause overlay
#include "CombatScene.hpp"
#include "SceneContext.hpp"
#include "core/Constants.hpp"
#include "core/Worlds.hpp"
#include "renderer/HUD.hpp"
#include "renderer/BuildChart.hpp"
#include "systems/ModSystem.hpp"
#include "audio/AudioSystem.hpp"

#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>
#include <string>

void CombatScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;

    if (m_shakeTimer > 0.f && m_shakeDuration > 0.f) {
        float mag = 14.f * (m_shakeTimer / m_shakeDuration);
        static thread_local std::mt19937 srng(12345u);
        std::uniform_int_distribution<int> sd(-(int)mag, (int)mag);
        SDL_Rect vp = { sd(srng), sd(srng), Constants::SCREEN_W, Constants::SCREEN_H };
        SDL_RenderSetViewport(ctx.renderer, &vp);
    }

    vr->clear();

    const WorldTheme& wt = worldDef(ctx.currentWorld).theme;
    float tierBoost = (m_config.tier - 1) * 0.15f;
    uint8_t gr = (uint8_t)(wt.gridR + wt.gridR * tierBoost);
    uint8_t gg = (uint8_t)(wt.gridG + wt.gridG * tierBoost);
    uint8_t gb = (uint8_t)(wt.gridB + wt.gridB * tierBoost);
    vr->drawGrid(gr, gg, gb);

    m_walls.render(vr, wt.accentR / 2, wt.accentG / 2, wt.accentB / 2);

    GlowColor projColor   = { Constants::COL_PROJ_R,   Constants::COL_PROJ_G,   Constants::COL_PROJ_B   };
    GlowColor avatarColor = m_isGolden
        ? GlowColor{ 255, 210, 30 }
        : GlowColor{ Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B };

    // Shield ring
    if (m_avatar && m_avatar->shielded) {
        GlowColor sc = { 80, 180, 255 };
        float r = m_avatar->radius + 8.f;
        constexpr int SEG = 20;
        for (int i = 0; i < SEG; ++i) {
            float a0 = i       * 6.28318f / SEG;
            float a1 = (i + 1) * 6.28318f / SEG;
            Vec2 p0 = m_avatar->pos + Vec2{ std::cos(a0) * r, std::sin(a0) * r };
            Vec2 p1 = m_avatar->pos + Vec2{ std::cos(a1) * r, std::sin(a1) * r };
            vr->drawGlowLine(p0, p1, sc);
        }
    }

    for (auto& p : m_projectiles) {
        if (p->radius >= 9.f) {
            // Iron Coffin heavy round — chunky orange-gold diamond
            float r = p->radius * 0.75f;
            GlowColor bigCol = { 255, 160, 30 };
            std::vector<Vec2> diamond = {
                { p->pos.x,     p->pos.y - r },
                { p->pos.x + r, p->pos.y     },
                { p->pos.x,     p->pos.y + r },
                { p->pos.x - r, p->pos.y     },
            };
            vr->drawGlowPoly(diamond, bigCol);
            // second inner diamond for visual weight
            float ri = r * 0.45f;
            std::vector<Vec2> inner = {
                { p->pos.x,      p->pos.y - ri },
                { p->pos.x + ri, p->pos.y      },
                { p->pos.x,      p->pos.y + ri },
                { p->pos.x - ri, p->pos.y      },
            };
            vr->drawGlowPoly(inner, { 255, 220, 100 });
        } else {
            const auto& v = p->worldVerts();
            if (v.size() >= 2) vr->drawGlowLine(v[0], v[1], projColor);
        }
    }

    renderICE(ctx);
    renderMines(ctx);

    // DataPacket render
    if (m_config.objective == NodeObjective::Extract && ctx.vrenderer) {
        for (const auto& dp : m_dataPackets) {
            if (!dp.alive) continue;
            float r = dp.radius;
            Vec2 verts[4] = {
                { dp.pos.x,     dp.pos.y - r },
                { dp.pos.x + r, dp.pos.y     },
                { dp.pos.x,     dp.pos.y + r },
                { dp.pos.x - r, dp.pos.y     },
            };
            float pulse = std::abs(std::sin(dp.lifetime * 4.f));
            uint8_t bright = static_cast<uint8_t>(160 + 95 * pulse);
            GlowColor dataCol = { 40, bright, static_cast<uint8_t>(bright / 2) };
            std::vector<Vec2> poly(std::begin(verts), std::end(verts));
            vr->drawGlowPoly(poly, dataCol);
            vr->drawGlowLine({ dp.pos.x - r * 0.5f, dp.pos.y },
                             { dp.pos.x + r * 0.5f, dp.pos.y }, dataCol);
            vr->drawGlowLine({ dp.pos.x, dp.pos.y - r * 0.5f },
                             { dp.pos.x, dp.pos.y + r * 0.5f }, dataCol);
        }
    }

    // Boss render
    if (m_boss && m_boss->alive) {
        static float s_time = 0.f;
        s_time += 0.016f;
        m_boss->render(vr, s_time);
    }

    if (m_avatar && m_avatar->alive)
        vr->drawGlowPoly(m_avatar->worldVerts(), avatarColor);

    // GRAVITY_WELL — concentric rings contracting toward center
    if (m_gravTimer > 0.f) {
        SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
        float phase = m_gravTimer * 2.5f;
        Vec2 center = { Constants::SCREEN_WF * 0.5f, Constants::SCREEN_HF * 0.5f };
        for (int ring = 0; ring < 4; ++ring) {
            float frac = std::fmod(phase + ring * 0.25f, 1.f);
            float r    = frac * 340.f;
            uint8_t alpha = (uint8_t)(120 * (1.f - frac));
            SDL_SetRenderDrawColor(ctx.renderer, 160, 40, 255, alpha);
            constexpr int SEG = 28;
            for (int i = 0; i < SEG; ++i) {
                float a0 = i       * 6.28318f / SEG;
                float a1 = (i + 1) * 6.28318f / SEG;
                SDL_RenderDrawLine(ctx.renderer,
                    (int)(center.x + std::cos(a0) * r), (int)(center.y + std::sin(a0) * r),
                    (int)(center.x + std::cos(a1) * r), (int)(center.y + std::sin(a1) * r));
            }
        }
    }

    // BEACON — glowing auto-turret diamond at drop position
    if (m_beaconTimer > 0.f) {
        float pulse = 0.6f + 0.4f * std::sin(SDL_GetTicks() * 0.007f);
        float fade  = std::min(m_beaconTimer / 2.f, 1.f);
        GlowColor bc = { (uint8_t)(255 * pulse * fade),
                          (uint8_t)(160 * pulse * fade),
                          (uint8_t)(20  * fade) };
        float s = 10.f;
        std::vector<Vec2> diamond = {
            { m_beaconPos.x,     m_beaconPos.y - s },
            { m_beaconPos.x + s, m_beaconPos.y     },
            { m_beaconPos.x,     m_beaconPos.y + s },
            { m_beaconPos.x - s, m_beaconPos.y     },
        };
        vr->drawGlowPoly(diamond, bc);
        // targeting cross
        GlowColor crossC = { 255, 200, 60 };
        vr->drawGlowLine({ m_beaconPos.x - 16.f, m_beaconPos.y },
                         { m_beaconPos.x + 16.f, m_beaconPos.y }, crossC);
        vr->drawGlowLine({ m_beaconPos.x, m_beaconPos.y - 16.f },
                         { m_beaconPos.x, m_beaconPos.y + 16.f }, crossC);
    }

    // Drones — small glowing triangles orbiting the avatar
    if (m_avatar && m_avatar->alive && !m_drones.empty()) {
        constexpr float ORBIT_R = 50.f;
        for (const auto& d : m_drones) {
            float fade  = std::min(d.life / 2.f, 1.f);
            float pulse = 0.6f + 0.4f * std::sin(SDL_GetTicks() * 0.008f + d.orbitAngle);
            GlowColor dc = { (uint8_t)(255 * fade * pulse),
                              (uint8_t)(200 * fade * pulse),
                              (uint8_t)(60  * fade) };
            float dx = std::cos(d.orbitAngle) * ORBIT_R;
            float dy = std::sin(d.orbitAngle) * ORBIT_R;
            Vec2 dp = { m_avatar->pos.x + dx, m_avatar->pos.y + dy };
            float s  = 8.f;
            float fa = d.orbitAngle + 1.5708f;  // face tangent direction
            std::vector<Vec2> tri = {
                { dp.x + std::cos(fa)         * s,     dp.y + std::sin(fa)         * s     },
                { dp.x + std::cos(fa + 2.09f) * s,     dp.y + std::sin(fa + 2.09f) * s     },
                { dp.x + std::cos(fa + 4.19f) * s,     dp.y + std::sin(fa + 4.19f) * s     },
            };
            vr->drawGlowPoly(tri, dc);
            // orbit ring segment hint
            vr->drawGlowLine(m_avatar->pos, dp, { (uint8_t)(80 * fade), (uint8_t)(60 * fade), 0 });
        }
    }

    // CLONE decoy
    if (m_decoyTimer > 0.f && m_avatar) {
        float fade = std::min(m_decoyTimer / 4.f, 1.f);
        float dpulse = 0.4f + 0.3f * std::sin(SDL_GetTicks() * 0.006f);
        GlowColor dc = { (uint8_t)(60 * fade * dpulse),
                          (uint8_t)(200 * fade * dpulse),
                          (uint8_t)(255 * fade * dpulse) };
        auto verts = m_avatar->worldVerts();
        Vec2 off = m_decoyPos - m_avatar->pos;
        for (auto& v : verts) { v.x += off.x; v.y += off.y; }
        vr->drawGlowPoly(verts, dc);
    }

    if (m_empTimer > 0.f) {
        SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctx.renderer, 0, 200, 200, 18);
        SDL_RenderFillRect(ctx.renderer, nullptr);
    }

    m_particles.render(ctx.renderer);
    m_fragments.render(ctx.renderer);

    vr->drawCRTOverlay();

    // Threshold crossing flash
    if (m_thresholdFlash > 0.f) {
        float t = std::max(0.f, m_thresholdFlash);
        uint8_t fr = 255, fg = 60, fb = 20;
        if (m_thresholdFlashPct >= 100) { fr=255; fg=0;  fb=0;   }
        else if (m_thresholdFlashPct >= 75)  { fr=220; fg=0;  fb=255; }
        else if (m_thresholdFlashPct >= 50)  { fr=255; fg=120;fb=0;   }
        else                                 { fr=255; fg=220;fb=0;   }
        SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctx.renderer, fr, fg, fb, (uint8_t)(55 * t));
        SDL_RenderFillRect(ctx.renderer, nullptr);
        SDL_SetRenderDrawColor(ctx.renderer, fr, fg, fb, (uint8_t)(120 * t));
        SDL_Rect border = { 0, 0, Constants::SCREEN_W, Constants::SCREEN_H };
        for (int i = 0; i < 4; ++i) {
            SDL_RenderDrawRect(ctx.renderer, &border);
            border = { border.x+1, border.y+1, border.w-2, border.h-2 };
        }
    }

    float glitchIntensity = std::max(0.f, (m_trace.trace() - 75.f) / 25.f);
    if (glitchIntensity > 0.f) vr->drawGlitch(glitchIntensity, SDL_GetTicks());

    bool hasController = (ctx.controller != nullptr);
    if (ctx.hud) ctx.hud->render(m_score, m_trace.trace(), ctx.programs, ctx.mods, hasController);

    if (ctx.hud) {
        std::string objStr;
        if (m_config.endless) {
            objStr = "WAVE " + std::to_string(ctx.endlessWave)
                   + "  ICE: " + std::to_string(m_iceKilled % 10) + "/10";
        } else if (m_config.objective == NodeObjective::Survive) {
            int s = std::max(0, (int)m_surviveTimer + 1);
            objStr = "SURVIVE: " + std::to_string(s) + "s";
        } else if (m_config.objective == NodeObjective::Extract) {
            objStr = "DATA: " + std::to_string(m_packetsCollected)
                   + " / " + std::to_string(m_config.extractTarget);
        } else {
            objStr = "ICE: " + std::to_string(m_iceKilled)
                   + " / " + std::to_string(m_config.sweepTarget);
        }
        SDL_Color cyan = { 100, 220, 200, 220 };
        int objW = ctx.hud->measureText(objStr);
        ctx.hud->drawLabel(objStr, Constants::SCREEN_W / 2 - objW / 2,
                           Constants::SCREEN_H - 38, cyan);
    }

    // Boss health bar
    if (m_boss && m_boss->alive && ctx.hud)
        ctx.hud->drawBossBar(m_boss->bossName(), m_boss->hp, m_boss->maxHp);

    // Boss phase 2 transition banner
    if (m_bossPhaseTimer > 0.f && ctx.hud) {
        float t = std::min(1.f, m_bossPhaseTimer / 2.8f);
        float fade = (t > 0.5f) ? 1.f : (t / 0.5f);
        if (m_bossPhaseTimer < 0.6f) fade = m_bossPhaseTimer / 0.6f;
        uint8_t a = (uint8_t)(255 * fade);
        SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctx.renderer, 255, 30, 30, (uint8_t)(40 * fade));
        SDL_RenderFillRect(ctx.renderer, nullptr);
        SDL_SetRenderDrawColor(ctx.renderer, 255, 30, 30, (uint8_t)(180 * fade));
        SDL_Rect b = {0, 0, Constants::SCREEN_W, Constants::SCREEN_H};
        for (int i = 0; i < 3; ++i) { SDL_RenderDrawRect(ctx.renderer, &b); b={b.x+1,b.y+1,b.w-2,b.h-2}; }
        SDL_Color warn  = {255, 60,  60,  a};
        SDL_Color sub   = {220, 180, 180, a};
        std::string line1 = "-- PHASE 2 --";
        std::string line2 = "SYSTEM OVERRIDE";
        int cx = Constants::SCREEN_W / 2;
        int cy = Constants::SCREEN_H / 2 - 16;
        ctx.hud->drawLabel(line1, cx - ctx.hud->measureText(line1)/2, cy,      warn);
        ctx.hud->drawLabel(line2, cx - ctx.hud->measureText(line2)/2, cy + 22, sub);
    }

    if (m_shakeTimer > 0.f)
        SDL_RenderSetViewport(ctx.renderer, nullptr);

    if (m_paused)
        renderPauseOverlay(ctx);

    SDL_RenderPresent(ctx.renderer);
}

void CombatScene::renderICE(SceneContext& ctx) const {
    VectorRenderer* vr = ctx.vrenderer;
    GlowColor hunterColor  = { Constants::COL_HUNTER_R,  Constants::COL_HUNTER_G,  Constants::COL_HUNTER_B  };
    GlowColor sentryColor  = { Constants::COL_SENTRY_R,  Constants::COL_SENTRY_G,  Constants::COL_SENTRY_B  };
    GlowColor spawnerColor = { Constants::COL_SPAWNER_R, Constants::COL_SPAWNER_G, Constants::COL_SPAWNER_B };
    GlowColor eprojColor   = { Constants::COL_EPROJ_R,   Constants::COL_EPROJ_G,   Constants::COL_EPROJ_B   };

    for (auto& h  : m_hunters)  vr->drawGlowPoly(h->worldVerts(), hunterColor);
    for (auto& s  : m_sentries) {
        vr->drawGlowPoly(s->worldVerts(), sentryColor);
        GlowColor ringCol = { 100, 60, 0 };
        constexpr int RSEGS = 16;
        constexpr float RRAD = 26.f;
        for (int i = 0; i < RSEGS; ++i) {
            float a0 = i       * 6.28318f / RSEGS;
            float a1 = (i + 1) * 6.28318f / RSEGS;
            vr->drawGlowLine(
                s->pos + Vec2{ std::cos(a0) * RRAD, std::sin(a0) * RRAD },
                s->pos + Vec2{ std::cos(a1) * RRAD, std::sin(a1) * RRAD },
                ringCol);
        }
    }
    for (auto& sp : m_spawnerICE) {
        vr->drawGlowPoly(sp->worldVerts(), spawnerColor);
        const auto& v = sp->worldVerts();
        std::vector<Vec2> inner;
        inner.reserve(v.size());
        for (const auto& pt : v)
            inner.push_back(sp->pos + (pt - sp->pos) * 0.5f);
        vr->drawGlowPoly(inner, spawnerColor);
    }
    for (auto& ph : m_phantoms) {
        if (!ph->alive) continue;
        float vis = ph->visibility;
        if (ph->blinkTimer > 0.f) {
            float blink = std::sin(ph->blinkTimer * 50.27f);
            vis = blink > 0.f ? 1.f : 0.15f;
        }
        if (vis < 0.02f) continue;
        GlowColor phantomCol = { (uint8_t)(50 * vis), (uint8_t)(200 * vis), (uint8_t)(220 * vis) };
        vr->drawGlowPoly(ph->worldVerts(), phantomCol);
    }
    for (auto& lc : m_leeches) {
        if (!lc->alive) continue;
        float t = lc->attached ? (0.6f + 0.4f * std::sin(SDL_GetTicks() * 0.008f)) : 1.f;
        GlowColor leechCol = { (uint8_t)(180 * t), (uint8_t)(60 * t), (uint8_t)(255 * t) };
        vr->drawGlowPoly(lc->worldVerts(), leechCol);
    }
    for (auto& mi : m_mirrors) {
        if (!mi->alive) continue;
        bool flashing = mi->hitFlashTimer > 0.f;
        // Flash orange-red on hit, scale brightness with remaining HP
        float hpFrac = mi->hp / 3.f;
        uint8_t rb = flashing ? 255 : (uint8_t)(160 + 40 * hpFrac);
        uint8_t gb = flashing ? 120 : (uint8_t)(200 + 30 * hpFrac);
        uint8_t bb = flashing ?  60 : 255;
        GlowColor mirrorCol = { rb, gb, bb };
        vr->drawGlowPoly(mi->worldVerts(), mirrorCol);
        // HP tick marks on face line
        Vec2 perp = { -std::sin(mi->facingAngle), std::cos(mi->facingAngle) };
        Vec2 faceA = mi->pos + perp * (mi->radius * 0.85f);
        Vec2 faceB = mi->pos - perp * (mi->radius * 0.85f);
        GlowColor faceCol = flashing ? GlowColor{255, 120, 30} : GlowColor{255, 255, 255};
        vr->drawGlowLine(faceA, faceB, faceCol);
        // HP pips — one dot per remaining HP above base
        for (int h = 0; h < mi->hp - 1; ++h) {
            float t = (h + 0.5f) / 3.f - 0.25f;
            Vec2 pip = mi->pos + perp * (mi->radius * t);
            vr->drawGlowLine(pip + Vec2{-2.f, -2.f}, pip + Vec2{2.f, 2.f}, {255, 220, 80});
        }
    }
    for (auto& ep : m_enemyProjectiles) {
        const auto& v = ep->worldVerts();
        if (v.size() >= 2) vr->drawGlowLine(v[0], v[1], eprojColor);
    }
}

// ---------------------------------------------------------------------------
// Mine rendering
// ---------------------------------------------------------------------------

void CombatScene::renderMines(SceneContext& ctx) const {
    SDL_Renderer* r = ctx.renderer;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (const auto& mine : m_mines) {
        if (!mine->alive) continue;

        GlowColor mineCol = { 160, 255, 0 };
        ctx.vrenderer->drawGlowPoly(mine->worldVerts(), mineCol);
        GlowColor crossCol = { 80, 200, 0 };
        float mcx = mine->pos.x, mcy = mine->pos.y;
        ctx.vrenderer->drawGlowLine({mcx - 7.f, mcy}, {mcx + 7.f, mcy}, crossCol);
        ctx.vrenderer->drawGlowLine({mcx, mcy - 7.f}, {mcx, mcy + 7.f}, crossCol);

        for (int i = 0; i < mine->hitsLeft; ++i) {
            SDL_SetRenderDrawColor(r, 160, 255, 40, 200);
            SDL_Rect dot = { (int)mine->pos.x - 8 + i * 8, (int)mine->pos.y + 14, 4, 4 };
            SDL_RenderFillRect(r, &dot);
        }

        float warnFrac = mine->warningFrac();
        if (warnFrac > 0.f) {
            float ringR   = mine->pulseRadius();
            int   segs    = 24;
            uint8_t alpha = (uint8_t)(200 * warnFrac);
            SDL_SetRenderDrawColor(r, 255, 80, 0, alpha);
            for (int i = 0; i < segs; ++i) {
                float a0 = i       * 6.28318f / segs;
                float a1 = (i + 1) * 6.28318f / segs;
                SDL_RenderDrawLine(r,
                    (int)(mine->pos.x + std::cos(a0) * ringR),
                    (int)(mine->pos.y + std::sin(a0) * ringR),
                    (int)(mine->pos.x + std::cos(a1) * ringR),
                    (int)(mine->pos.y + std::sin(a1) * ringR));
            }
        }

        if (mine->state() == PulseState::Pulse) {
            float R = PulseMine::DAMAGE_RADIUS;
            SDL_SetRenderDrawColor(r, 255, 100, 0, 50);
            SDL_Rect flash = {
                (int)(mine->pos.x - R), (int)(mine->pos.y - R),
                (int)(R * 2),           (int)(R * 2)
            };
            SDL_RenderFillRect(r, &flash);
        }
    }
}

// ---------------------------------------------------------------------------
// Pause overlay
// ---------------------------------------------------------------------------

static std::string pauseVolBar(int vol) {
    int filled = vol / 10;
    std::string bar = "[";
    for (int i = 0; i < 10; ++i) bar += (i < filled ? "=" : " ");
    bar += "] " + std::to_string(vol) + "%";
    return bar;
}

void CombatScene::pauseChangeVolume(int item, int delta, SceneContext& ctx) {
    if (item == 1) {
        m_pauseMusicVol = std::max(0, std::min(100, m_pauseMusicVol + delta * 10));
        if (ctx.audio) ctx.audio->setMusicVolume(m_pauseMusicVol * MIX_MAX_VOLUME / 100);
    } else if (item == 2) {
        m_pauseSfxVol = std::max(0, std::min(100, m_pauseSfxVol + delta * 10));
        if (ctx.audio) ctx.audio->setSfxVolume(m_pauseSfxVol * MIX_MAX_VOLUME / 100);
    }
}

void CombatScene::renderPauseOverlay(SceneContext& ctx) const {
    SDL_Renderer* r = ctx.renderer;
    if (!r || !ctx.hud) return;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(r, 0, 0, 0, 170);
    SDL_RenderFillRect(r, nullptr);

    float pulse = 0.5f + 0.5f * std::sin(m_pauseTime * 3.f);

    constexpr int MENU_W  = 340;
    constexpr int CHART_W = 280;
    constexpr int GAP     = 16;
    constexpr int PANEL_H = 320;
    constexpr int TOTAL_W = MENU_W + GAP + CHART_W;

    int panelX = Constants::SCREEN_W / 2 - TOTAL_W / 2;
    int panelY = Constants::SCREEN_H / 2 - PANEL_H / 2;
    int chartX = panelX + MENU_W + GAP;

    SDL_SetRenderDrawColor(r, 0, 14, 10, 230);
    SDL_Rect menuBg = { panelX, panelY, MENU_W, PANEL_H };
    SDL_RenderFillRect(r, &menuBg);
    SDL_SetRenderDrawColor(r, 0, (Uint8)(180 + 60 * pulse), 120, 220);
    SDL_RenderDrawRect(r, &menuBg);

    SDL_Color titleCol = { 0, (Uint8)(210 + 45 * pulse), 150, 255 };
    ctx.hud->drawLabel("// PAUSED //", panelX + 16, panelY + 14, titleCol);
    SDL_SetRenderDrawColor(r, 0, 80, 60, 140);
    SDL_RenderDrawLine(r, panelX+12, panelY+44, panelX+MENU_W-12, panelY+44);

    struct Entry { const char* label; std::string value; };
    Entry entries[] = {
        { "RESUME",       "" },
        { "MUSIC VOLUME", pauseVolBar(m_pauseMusicVol) },
        { "SFX VOLUME",   pauseVolBar(m_pauseSfxVol)   },
        { "QUIT TO MENU", "" },
    };

    int ry = panelY + 54;
    for (int i = 0; i < 4; ++i) {
        bool sel = (m_pauseCursor == i);
        float p  = sel ? pulse : 0.f;
        SDL_Color labelCol = sel ? SDL_Color{0,(Uint8)(200+40*p),(Uint8)(160+60*p),255}
                                 : SDL_Color{70, 130, 100, 200};
        SDL_Color valCol   = sel ? SDL_Color{220, 220, 80, 255}
                                 : SDL_Color{150, 200, 80, 200};

        bool isVol = !entries[i].value.empty();
        int rowH = isVol ? 46 : 26;
        if (sel) {
            SDL_SetRenderDrawColor(r, 0, (Uint8)(30+15*p), (Uint8)(22+10*p), 120);
            SDL_Rect row = { panelX+8, ry-3, MENU_W-16, rowH };
            SDL_RenderFillRect(r, &row);
        }

        std::string lbl = sel ? ("> " + std::string(entries[i].label))
                               : ("  " + std::string(entries[i].label));
        ctx.hud->drawLabel(lbl.c_str(), panelX+16, ry, labelCol);
        if (isVol) {
            ctx.hud->drawLabel(entries[i].value.c_str(), panelX+26, ry+22, valCol);
            ry += 62;
        } else {
            ry += 44;
        }
    }

    SDL_Color hintCol = { 50, 90, 70, 160 };
    ctx.hud->drawLabel("ESC/START: resume  L/R: vol",
                       panelX+12, panelY+PANEL_H-22, hintCol);

    renderBuildChart(r, ctx.hud, ctx.mods, ctx.programs, ctx.saveData,
                     pulse, chartX, panelY, CHART_W, PANEL_H);
}
