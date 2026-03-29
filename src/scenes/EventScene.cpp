#include "EventScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "MapScene.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/HUD.hpp"
#include "renderer/VectorRenderer.hpp"
#include "world/NodeMap.hpp"

#include <SDL.h>
#include <cmath>
#include <random>
#include <sstream>

EventScene::EventScene(int nodeId, unsigned int seed)
    : m_nodeId(nodeId), m_seed(seed) {}

void EventScene::onEnter(SceneContext& ctx) {
    // Pick event deterministically from seed (node-based, not clock-based)
    std::mt19937 rng(m_seed ^ static_cast<unsigned>(m_nodeId * 2654435761u));
    std::uniform_int_distribution<int> dist(0, EVENT_POOL_SIZE - 1);
    m_event = &EVENT_POOL[dist(rng)];

    if (!m_applied) {
        m_applied = true;

        // Apply credit delta (clamp to 0 floor so we never go negative)
        if (m_event->creditDelta != 0 && ctx.saveData) {
            ctx.saveData->credits = std::max(0, ctx.saveData->credits + m_event->creditDelta);
            SaveSystem::save(*ctx.saveData);
        }

        // Apply trace penalty for next combat node
        ctx.nextNodeStartTrace += m_event->tracePenalty;

        // Apply bonus lives for next combat node
        ctx.bonusLives += m_event->livesBonus;

        // Mark this node cleared and unlock adjacent nodes
        if (ctx.nodeMap) ctx.nodeMap->clearNode(m_nodeId);
    }

    // Small delay before the player can dismiss (prevents holding Enter from skipping)
    m_canConfirm = false;
}

void EventScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (!m_canConfirm) return;

    bool pressed = false;
    if (ev.type == SDL_KEYDOWN || ev.type == SDL_MOUSEBUTTONDOWN)
        pressed = true;
    else if (ev.type == SDL_CONTROLLERBUTTONDOWN)
        pressed = (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                   ev.cbutton.button == SDL_CONTROLLER_BUTTON_START);

    if (pressed)
        ctx.scenes->replace(std::make_unique<MapScene>(*ctx.nodeMap));
}

void EventScene::update(float dt, SceneContext& ctx) {
    (void)ctx;
    m_timer += dt;
    if (m_timer > 0.6f) m_canConfirm = true;
}

void EventScene::render(SceneContext& ctx) {
    SDL_Renderer* r = ctx.renderer;
    if (!r) return;

    // Background
    ctx.vrenderer->clear();
    ctx.vrenderer->drawGrid(20, 50, 80);
    ctx.vrenderer->drawCRTOverlay();

    if (!ctx.hud || !m_event) { SDL_RenderPresent(r); return; }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Colour scheme: buff=cyan-green, debuff=orange-red
    bool isBuff    = (m_event->creditDelta >= 0 && m_event->tracePenalty == 0.f);
    uint8_t boxR   = isBuff ?  0 : 40;
    uint8_t boxG   = isBuff ? 25 :  8;
    uint8_t boxB   = isBuff ? 15 :  0;
    uint8_t borR   = isBuff ?  0 : 255;
    uint8_t borG   = isBuff ? 200 : 140;
    uint8_t borB   = isBuff ? 100 :  20;

    // Animated slide-in
    float slideT = std::min(1.f, m_timer / 0.25f);
    float ease   = 1.f - (1.f - slideT) * (1.f - slideT);

    int boxW = 520, boxH = 280;
    int bx   = Constants::SCREEN_W / 2 - boxW / 2;
    int byFull = Constants::SCREEN_H / 2 - boxH / 2;
    int by   = byFull + static_cast<int>((1.f - ease) * 40.f);

    // Box fill + border
    SDL_Rect bg = { bx, by, boxW, boxH };
    SDL_SetRenderDrawColor(r, boxR, boxG, boxB, 220);
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, borR, borG, borB, 240);
    SDL_RenderDrawRect(r, &bg);

    // Second inner border for depth
    SDL_Rect inner = { bx+2, by+2, boxW-4, boxH-4 };
    SDL_SetRenderDrawColor(r, borR/3, borG/3, borB/3, 160);
    SDL_RenderDrawRect(r, &inner);

    // Divider lines
    SDL_SetRenderDrawColor(r, borR/2, borG/2, borB/2, 140);
    SDL_RenderDrawLine(r, bx+12, by+54,  bx+boxW-12, by+54);
    SDL_RenderDrawLine(r, bx+12, by+206, bx+boxW-12, by+206);

    // Header: ">> NODE TYPE <<"
    SDL_Color headCol = { borR, borG, borB, 255 };
    std::string head  = std::string(">> ") + m_event->title + " <<";
    int hw = ctx.hud->measureText(head);
    ctx.hud->drawLabel(head, bx + boxW/2 - hw/2, by + 16, headCol);

    // Body text — split on \n, then word-wrap each segment to fit the card
    SDL_Color bodyCol = { 160, 200, 170, 230 };
    if (!isBuff) bodyCol = { 200, 160, 120, 230 };
    std::string bodyStr = m_event->body;
    int lineY = by + 68;
    const int maxTextW = boxW - 32;
    std::istringstream ss(bodyStr);
    std::string seg;
    while (std::getline(ss, seg)) {
        int drawn = ctx.hud->drawWrapped(seg, bx + 16, lineY, maxTextW, bodyCol, 26);
        lineY += drawn * 26;
    }

    // Outcome
    SDL_Color outCol = isBuff ? SDL_Color{ 80, 240, 160, 255 }
                              : SDL_Color{ 255, 100,  60, 255 };
    ctx.hud->drawLabel(m_event->outcome, bx + 16, by + 216, outCol);

    // Prompt (blink after delay)
    if (m_canConfirm) {
        float blink = std::fmod(m_timer, 0.9f);
        if (blink < 0.6f) {
            SDL_Color promptCol = { borR, borG, borB, 200 };
            std::string prompt  = ">> PRESS ANY KEY TO CONTINUE <<";
            int pw = ctx.hud->measureText(prompt);
            ctx.hud->drawLabel(prompt, bx + boxW/2 - pw/2, by + boxH - 28, promptCol);
        }
    }

    SDL_RenderPresent(r);
}
