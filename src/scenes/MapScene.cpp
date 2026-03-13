#include "MapScene.hpp"
#include "CombatScene.hpp"
#include "EventScene.hpp"
#include "SceneContext.hpp"
#include "systems/ModSystem.hpp"
#include "core/Mods.hpp"
#include "core/Worlds.hpp"
#include "SceneManager.hpp"
#include "renderer/HUD.hpp"
#include "core/Constants.hpp"
#include "world/Node.hpp"

#include <SDL.h>
#include <cmath>
#include <algorithm>
#include <climits>

// ---------------------------------------------------------------------------
// Room layout constants
// ---------------------------------------------------------------------------
static constexpr int ROOM_W    = 70;
static constexpr int ROOM_H    = 40;
static constexpr int GRID_DX   = 100;  // pixels between room columns
static constexpr int GRID_DY   = 65;   // pixels between room rows

// Map origin (top-left of col=0,row=0 cell)
static constexpr int MAP_ORIGIN_X = Constants::SCREEN_W / 2 - GRID_DX - ROOM_W / 2;
static constexpr int MAP_ORIGIN_Y = Constants::SCREEN_H / 2 - 2 * GRID_DY - ROOM_H / 2;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void roomRect(const Node& n, int& x, int& y, int& w, int& h) {
    x = MAP_ORIGIN_X + n.gridCol * GRID_DX;
    y = MAP_ORIGIN_Y + n.gridRow * GRID_DY;
    w = ROOM_W;
    h = ROOM_H;
}

static void roomCenter(const Node& n, int& cx, int& cy) {
    int rx, ry, rw, rh;
    roomRect(n, rx, ry, rw, rh);
    cx = rx + rw / 2;
    cy = ry + rh / 2;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

MapScene::MapScene(NodeMap& nodeMap) : m_nodeMap(nodeMap) {
    m_cursor = nodeMap.startNode();
}

void MapScene::onEnter(SceneContext&) {
    // Ensure cursor is on a reachable node
    if (m_nodeMap.node(m_cursor).status == NodeStatus::Locked)
        m_cursor = m_nodeMap.startNode();
}

void MapScene::onExit() {}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void MapScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    int dCol = 0, dRow = 0;

    if (ev.type == SDL_KEYDOWN) {
        const auto sc = ev.key.keysym.scancode;
        if      (sc == SDL_SCANCODE_LEFT  || sc == SDL_SCANCODE_A) dCol = -1;
        else if (sc == SDL_SCANCODE_RIGHT || sc == SDL_SCANCODE_D) dCol =  1;
        else if (sc == SDL_SCANCODE_UP    || sc == SDL_SCANCODE_W) dRow = -1;
        else if (sc == SDL_SCANCODE_DOWN  || sc == SDL_SCANCODE_S) dRow =  1;
        else if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) {
            jackIn(ctx); return;
        }
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        const auto btn = ev.cbutton.button;
        if      (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT)  dCol = -1;
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) dCol =  1;
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_UP)    dRow = -1;
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN)  dRow =  1;
        else if (btn == SDL_CONTROLLER_BUTTON_A || btn == SDL_CONTROLLER_BUTTON_START) {
            jackIn(ctx); return;
        }
    }

    if (dCol != 0 || dRow != 0) moveCursor(dCol, dRow);
}

void MapScene::moveCursor(int dCol, int dRow) {
    const Node& cur = m_nodeMap.node(m_cursor);
    int bestId   = -1;
    int bestPri  = INT_MAX; // lower is better
    int bestSec  = INT_MAX;

    for (int nb : cur.connections) {
        const Node& n = m_nodeMap.node(nb);
        int dc = n.gridCol - cur.gridCol;
        int dr = n.gridRow - cur.gridRow;

        // Primary axis must match direction
        if (dCol != 0 && std::abs(dc) == 0) continue;
        if (dRow != 0 && std::abs(dr) == 0) continue;
        if (dCol != 0 && (dc > 0) != (dCol > 0)) continue;
        if (dRow != 0 && (dr > 0) != (dRow > 0)) continue;

        int pri = (dCol != 0) ? std::abs(dc) : std::abs(dr);
        int sec = (dCol != 0) ? std::abs(dr) : std::abs(dc);

        if (pri < bestPri || (pri == bestPri && sec < bestSec)) {
            bestPri = pri;
            bestSec = sec;
            bestId  = nb;
        }
    }

    if (bestId != -1) m_cursor = bestId;
}

bool MapScene::jackIn(SceneContext& ctx) {
    const Node& n = m_nodeMap.node(m_cursor);
    if (n.status != NodeStatus::Available) return false;

    // Event nodes skip combat entirely
    if (n.objective == NodeObjective::Event) {
        // Seed from node id + world so the event is consistent if re-visited before clearing
        unsigned int seed = static_cast<unsigned>(ctx.currentWorld * 1000 + n.id);
        ctx.scenes->replace(std::make_unique<EventScene>(n.id, seed));
        return true;
    }

    // Compute trace tick rate from tier, scaled by world difficulty
    float worldMult = worldDef(ctx.currentWorld).diffMult;
    float tickRate  = (2.5f + (n.tier - 1) * 1.5f) * worldMult;

    NodeConfig cfg;
    cfg.nodeId         = n.id;
    cfg.tier           = n.tier;
    cfg.objective      = n.objective;
    cfg.sweepTarget    = n.sweepTarget;
    cfg.surviveSeconds = n.surviveSeconds;
    cfg.extractTarget  = n.extractTarget;
    cfg.traceTickRate  = tickRate;

    // Consume any pending event effects
    if (ctx.nextNodeStartTrace > 0.f) {
        cfg.startTrace          = ctx.nextNodeStartTrace;
        ctx.nextNodeStartTrace  = 0.f;
    }

    ctx.scenes->replace(std::make_unique<CombatScene>(cfg));
    return true;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void MapScene::update(float dt, SceneContext&) {
    m_pulse += dt * 3.f;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void MapScene::render(SceneContext& ctx) {
    SDL_Renderer* r = ctx.renderer;

    // Background
    SDL_SetRenderDrawColor(r, 2, 5, 12, 255);
    SDL_RenderClear(r);

    // Subtle grid
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 10, 40, 60, 18);
    for (int x = 0; x < Constants::SCREEN_W; x += 80)
        SDL_RenderDrawLine(r, x, 0, x, Constants::SCREEN_H);
    for (int y = 0; y < Constants::SCREEN_H; y += 80)
        SDL_RenderDrawLine(r, 0, y, Constants::SCREEN_W, y);

    drawCorridors(r);

    const auto& nodes = m_nodeMap.nodes();
    for (const auto& n : nodes) {
        drawRoom(r, n, (n.id == m_cursor), m_pulse);
    }

    // Header + selected-node info
    if (ctx.hud) {
        {
            const char* t = "// SYSTEM MAP //";
            ctx.hud->drawLabel(t, Constants::SCREEN_W/2 - ctx.hud->measureText(t)/2, 18,
                               {0,220,180,255});
        }

        const Node& sel = m_nodeMap.node(m_cursor);
        bool selKnown = (sel.status != NodeStatus::Locked || sel.revealed);
        std::string nodeInfo = selKnown ? sel.name + "  " : "???  ";
        if (!selKnown)                                     nodeInfo += "[UNKNOWN]";
        else if (sel.objective == NodeObjective::Sweep)    nodeInfo += "[SWEEP]";
        else if (sel.objective == NodeObjective::Survive)  nodeInfo += "[SURVIVE]";
        else if (sel.objective == NodeObjective::Extract)  nodeInfo += "[EXTRACT]";
        else if (sel.objective == NodeObjective::Event)    nodeInfo += "[EVENT]";
        else if (sel.objective == NodeObjective::Boss)     nodeInfo += "[BOSS]";

        std::string statusStr;
        if      (sel.status == NodeStatus::Cleared)        statusStr = "CLEARED";
        else if (sel.status == NodeStatus::Available)      statusStr = "AVAILABLE  --  ENTER to jack in";
        else if (sel.revealed)                             statusStr = "LOCKED  --  clear an adjacent node to access";
        else                                               statusStr = "LOCKED";

        // Anchor at SCREEN_H-28 (top of instruction bar) and draw upward
        ctx.hud->drawLabel(nodeInfo,   Constants::SCREEN_W/2 - 180, Constants::SCREEN_H - 80, {100,200,160,200});
        ctx.hud->drawLabel(statusStr,  Constants::SCREEN_W/2 - 180, Constants::SCREEN_H - 54, {60,140,110,200});

        // Mod inventory — top right
        if (ctx.mods && !ctx.mods->all().empty()) {
            const auto& mods = ctx.mods->all();
            ctx.hud->drawLabel("INSTALLED:", Constants::SCREEN_W - 8 - ctx.hud->measureText("INSTALLED:"), 18, {120,160,140,200});
            // Count unique mods
            std::vector<std::pair<ModID,int>> unique;
            for (ModID m : mods) {
                bool found = false;
                for (auto& p : unique) if (p.first == m) { p.second++; found = true; break; }
                if (!found) unique.push_back({m, 1});
            }
            for (int i = 0; i < (int)unique.size() && i < 6; ++i) {
                const ModDef& def = getModDef(unique[i].first);
                std::string label = def.name;
                if (unique[i].second > 1) label += " x" + std::to_string(unique[i].second);
                uint8_t mr, mg, mb;
                switch (def.category) {
                    case ModCategory::Chassis: mr=40;  mg=200; mb=120; break;
                    case ModCategory::Weapon:  mr=220; mg=180; mb=30;  break;
                    case ModCategory::Neural:  mr=180; mg=60;  mb=255; break;
                    default:                   mr=140; mg=140; mb=140; break;
                }
                int lw = ctx.hud->measureText(label);
                ctx.hud->drawLabel(label, Constants::SCREEN_W - 8 - lw, 40 + i * 22,
                                   {mr, mg, mb, 200});
            }
        }
    }

    // Header underline
    SDL_SetRenderDrawColor(r, 0, 180, 140, 80);
    SDL_RenderDrawLine(r, Constants::SCREEN_W/2 - 120, 48,
                          Constants::SCREEN_W/2 + 120, 48);

    // Instruction bar at bottom
    SDL_SetRenderDrawColor(r, 40, 60, 80, 160);
    SDL_Rect bar = { 0, Constants::SCREEN_H - 28, Constants::SCREEN_W, 28 };
    SDL_RenderFillRect(r, &bar);
    SDL_SetRenderDrawColor(r, 0, 180, 140, 120);
    SDL_RenderDrawLine(r, 0, Constants::SCREEN_H - 28,
                          Constants::SCREEN_W, Constants::SCREEN_H - 28);

    SDL_RenderPresent(r);
}

void MapScene::drawCorridors(SDL_Renderer* r) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    const auto& nodes = m_nodeMap.nodes();
    for (const auto& n : nodes) {
        int ax, ay;
        roomCenter(n, ax, ay);

        for (int nb : n.connections) {
            if (nb <= n.id) continue; // draw each corridor once

            const Node& other = nodes[nb];
            int bx, by;
            roomCenter(other, bx, by);

            // Dim corridor if either end is locked
            bool dim = (n.status == NodeStatus::Locked || other.status == NodeStatus::Locked);
            SDL_SetRenderDrawColor(r, dim ? 30 : 0,
                                      dim ? 50 : 160,
                                      dim ? 60 : 120,
                                      dim ? 60 : 160);
            SDL_RenderDrawLine(r, ax, ay, bx, by);
            if (!dim) {
                // Double-draw for glow
                SDL_SetRenderDrawColor(r, 0, 80, 60, 40);
                SDL_RenderDrawLine(r, ax-1, ay, bx-1, by);
                SDL_RenderDrawLine(r, ax+1, ay, bx+1, by);
                SDL_RenderDrawLine(r, ax, ay-1, bx, by-1);
                SDL_RenderDrawLine(r, ax, ay+1, bx, by+1);
            }
        }
    }
}

void MapScene::drawRoom(SDL_Renderer* r, const Node& n, bool selected, float pulse) const {
    int rx, ry, rw, rh;
    roomRect(n, rx, ry, rw, rh);
    SDL_Rect room = { rx, ry, rw, rh };

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Fill color by status
    uint8_t fr = 0, fg = 0, fb = 0, fa = 200;
    uint8_t br = 0, bg = 0, bb = 0; // border
    switch (n.status) {
        case NodeStatus::Locked:
            if (n.revealed) {
                // Shadow Scanner: slightly brighter, teal-tinted — visible but not enterable
                fr=10; fg=30; fb=40; fa=185;
                br=40; bg=90; bb=110;
            } else {
                fr=8;  fg=8;  fb=15; fa=180;
                br=35; bg=35; bb=55;
            }
            break;
        case NodeStatus::Available:
            fr=5;  fg=55; fb=70; fa=200;
            br=20; bg=200;bb=170;
            break;
        case NodeStatus::Cleared:
            fr=5;  fg=50; fb=25; fa=180;
            br=0;  bg=140;bb=60;
            break;
    }

    // Selected: brighter fill + animated border
    if (selected) {
        float brightness = 0.55f + 0.45f * std::sin(pulse);
        fr = (uint8_t)std::min(255.f, fr + 30 * brightness);
        fg = (uint8_t)std::min(255.f, fg + 60 * brightness);
        fb = (uint8_t)std::min(255.f, fb + 80 * brightness);
        // Outer glow rects
        SDL_SetRenderDrawColor(r, 0, 200, 180,
            (uint8_t)(40 * (0.5f + 0.5f * std::sin(pulse))));
        SDL_Rect glow = { rx-3, ry-3, rw+6, rh+6 };
        SDL_RenderDrawRect(r, &glow);
        glow = { rx-2, ry-2, rw+4, rh+4 };
        SDL_RenderDrawRect(r, &glow);
    }

    SDL_SetRenderDrawColor(r, fr, fg, fb, fa);
    SDL_RenderFillRect(r, &room);

    SDL_SetRenderDrawColor(r, br, bg, bb, 240);
    SDL_RenderDrawRect(r, &room);

    // Tier indicator dots (right side)
    SDL_SetRenderDrawColor(r, 80, 180, 160, 200);
    if (n.status != NodeStatus::Locked || n.revealed) {
        SDL_SetRenderDrawColor(r, 80, 180, 160, n.revealed && n.status == NodeStatus::Locked ? 120 : 200);
        for (int t = 0; t < n.tier; ++t) {
            SDL_Rect dot = { rx + rw - 8, ry + 4 + t * 7, 3, 3 };
            SDL_RenderFillRect(r, &dot);
        }
    }

    // Vector icon — centered in room, dimmed if locked (revealed nodes show partial icon)
    uint8_t iconAlpha = (n.status == NodeStatus::Locked)
        ? (n.revealed ? 110 : 40)
        : (selected ? 240 : 180);
    int icx = rx + rw / 2;
    int icy = ry + rh / 2;

    if (n.status == NodeStatus::Cleared) {
        // Checkmark
        SDL_SetRenderDrawColor(r, 0, 200, 100, iconAlpha);
        SDL_RenderDrawLine(r, icx-8, icy,   icx-3, icy+6);
        SDL_RenderDrawLine(r, icx-3, icy+6, icx+8, icy-7);
        SDL_RenderDrawLine(r, icx-8, icy+1, icx-3, icy+7);
        SDL_RenderDrawLine(r, icx-3, icy+7, icx+8, icy-6);
    } else if (n.objective == NodeObjective::Sweep) {
        // Skull: dome + eyes + teeth
        SDL_SetRenderDrawColor(r, 255, 80, 80, iconAlpha);
        // Cranium dome (8-seg arc)
        for (int i = 0; i < 7; ++i) {
            float a0 = -3.14159f + i * 3.14159f / 7.f;
            float a1 = -3.14159f + (i+1) * 3.14159f / 7.f;
            SDL_RenderDrawLine(r, icx+(int)(8*std::cos(a0)), icy+(int)(6*std::sin(a0)),
                                  icx+(int)(8*std::cos(a1)), icy+(int)(6*std::sin(a1)));
        }
        // Jaw line
        SDL_RenderDrawLine(r, icx-6, icy+4, icx+6, icy+4);
        // Teeth
        for (int tx = -4; tx <= 4; tx += 4)
            SDL_RenderDrawLine(r, icx+tx, icy+4, icx+tx, icy+7);
        // Eyes (filled rects)
        SDL_Rect le = { icx-6, icy-2, 3, 3 }; SDL_RenderFillRect(r, &le);
        SDL_Rect re = { icx+3, icy-2, 3, 3 }; SDL_RenderFillRect(r, &re);
    } else if (n.objective == NodeObjective::Survive) {
        // Hourglass
        SDL_SetRenderDrawColor(r, 80, 180, 255, iconAlpha);
        // Top triangle
        SDL_RenderDrawLine(r, icx-8, icy-9, icx+8, icy-9);
        SDL_RenderDrawLine(r, icx-8, icy-9, icx,   icy);
        SDL_RenderDrawLine(r, icx+8, icy-9, icx,   icy);
        // Bottom triangle
        SDL_RenderDrawLine(r, icx,   icy,   icx-8, icy+9);
        SDL_RenderDrawLine(r, icx,   icy,   icx+8, icy+9);
        SDL_RenderDrawLine(r, icx-8, icy+9, icx+8, icy+9);
        // Sand fill lines (top)
        for (int s = 1; s <= 3; ++s) {
            int w = 8 - s*2; int y = icy - 9 + s*2;
            SDL_RenderDrawLine(r, icx-w, y, icx+w, y);
        }
    } else if (n.objective == NodeObjective::Extract) {
        // Data packet diamond with downward arrow
        SDL_SetRenderDrawColor(r, 40, 220, 160, iconAlpha);
        // Diamond outline
        SDL_RenderDrawLine(r, icx,    icy-8,  icx+7,  icy);
        SDL_RenderDrawLine(r, icx+7,  icy,    icx,    icy+8);
        SDL_RenderDrawLine(r, icx,    icy+8,  icx-7,  icy);
        SDL_RenderDrawLine(r, icx-7,  icy,    icx,    icy-8);
        // Inner cross
        SDL_RenderDrawLine(r, icx-4,  icy,    icx+4,  icy);
        SDL_RenderDrawLine(r, icx,    icy-4,  icx,    icy+4);
    } else if (n.objective == NodeObjective::Event) {
        // Question mark — unknown contents
        SDL_SetRenderDrawColor(r, 220, 200, 60, iconAlpha);
        // Arc of '?' (top curve: two lines approximating a curve)
        SDL_RenderDrawLine(r, icx-5, icy-8, icx+5, icy-8);
        SDL_RenderDrawLine(r, icx+5, icy-8, icx+6, icy-3);
        SDL_RenderDrawLine(r, icx+6, icy-3, icx,   icy+1);
        SDL_RenderDrawLine(r, icx,   icy+1, icx,   icy+4);
        // Dot
        SDL_Rect qdot = { icx-1, icy+6, 3, 3 };
        SDL_RenderFillRect(r, &qdot);
    } else if (n.objective == NodeObjective::Boss) {
        // Diamond with inner diamond (danger symbol)
        SDL_SetRenderDrawColor(r, 255, 60, 30, iconAlpha);
        SDL_RenderDrawLine(r, icx,    icy-10, icx+9,  icy);
        SDL_RenderDrawLine(r, icx+9,  icy,    icx,    icy+10);
        SDL_RenderDrawLine(r, icx,    icy+10, icx-9,  icy);
        SDL_RenderDrawLine(r, icx-9,  icy,    icx,    icy-10);
        SDL_RenderDrawLine(r, icx,    icy-5,  icx+4,  icy);
        SDL_RenderDrawLine(r, icx+4,  icy,    icx,    icy+5);
        SDL_RenderDrawLine(r, icx,    icy+5,  icx-4,  icy);
        SDL_RenderDrawLine(r, icx-4,  icy,    icx,    icy-5);
    }
}
