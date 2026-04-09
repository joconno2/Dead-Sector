#include "NodeCompleteScene.hpp"
#include "MapScene.hpp"
#include "CombatScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "world/NodeMap.hpp"
#include "systems/ModSystem.hpp"
#include "systems/ProgramSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "math/Vec2.hpp"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <string>

// ---------------------------------------------------------------------------
// Card layout
// ---------------------------------------------------------------------------
static constexpr int CARDS       = 3;
static constexpr int CARD_W      = 260;
static constexpr int CARD_H      = 230;
static constexpr int CARD_PAD    = 20;
static constexpr int CARD_MARGIN = 30;   // min margin on each side of screen
static constexpr int CARD_Y0     = (Constants::SCREEN_H - CARD_H) / 2 + 20;

// Returns card width and padding scaled to fit n cards on screen
static void cardLayout(int n, int& outW, int& outPad, int& outX0) {
    int maxW = Constants::SCREEN_W - 2 * CARD_MARGIN;
    int tot  = n * CARD_W + (n - 1) * CARD_PAD;
    if (tot <= maxW) {
        outW   = CARD_W;
        outPad = CARD_PAD;
    } else {
        // Shrink card width first, keep padding
        outPad = CARD_PAD;
        outW   = (maxW - (n - 1) * outPad) / n;
    }
    int finalTot = n * outW + (n - 1) * outPad;
    outX0 = (Constants::SCREEN_W - finalTot) / 2;
}

// ---------------------------------------------------------------------------
// Bonus pool for thematic credit bonuses
// ---------------------------------------------------------------------------
struct BonusDef { const char* name; int minAmt; int maxAmt; };
static const BonusDef BONUS_POOL[] = {
    {"CREDIT CARD DUMP",       20,  120},
    {"BANK TRANSFER SKIM",     30,  150},
    {"CRYPTO WALLET DRAIN",    15,  200},
    {"CORPORATE PAYROLL",      60,  300},
    {"DARK WEB SALE",          20,   90},
    {"BIOMETRIC BYPASS FEE",   25,  120},
    {"ZERO-DAY EXPLOIT",       40,  180},
    {"ICE BOUNTY",             10,   50},
    {"TRACE EVASION FEE",      15,   70},
    {"SWISS ACCOUNT SKIM",     35,  160},
    {"GHOST PROTOCOL PAYOUT",  20,  100},
    {"NEURAL LACE EXPORT",     30,  140},
    {"PROPRIETARY KEYS SOLD",  45,  220},
    {"RANSOMWARE COLLECT",     50,  250},
    {"SATELLITE UPLINK SOLD",  30,  120},
};
static constexpr int BONUS_POOL_SIZE = (int)(sizeof(BONUS_POOL) / sizeof(BONUS_POOL[0]));

static void generateBonuses(SceneContext& ctx, std::mt19937& rng) {
    // Generate 2-3 bonuses
    std::uniform_int_distribution<int> countDist(2, 3);
    int count = countDist(rng);

    std::uniform_int_distribution<int> idxDist(0, BONUS_POOL_SIZE - 1);
    for (int i = 0; i < count; ++i) {
        int idx = idxDist(rng);
        const BonusDef& bd = BONUS_POOL[idx];
        std::uniform_int_distribution<int> amtDist(bd.minAmt, bd.maxAmt);
        int amt = amtDist(rng);

        ctx.runCredits += amt;
        ctx.runBonuses.push_back({ std::string(bd.name), amt });
    }
}

// ---------------------------------------------------------------------------
NodeCompleteScene::NodeCompleteScene(int nodeId, int score, int iceKilled, int sweepTarget,
                                     bool endless, int endlessWave, float carryTrace)
    : m_nodeId(nodeId), m_score(score), m_iceKilled(iceKilled), m_sweepTarget(sweepTarget)
    , m_endless(endless), m_endlessWave(endlessWave), m_carryTrace(carryTrace)
{}

void NodeCompleteScene::onEnter(SceneContext& ctx) {
    m_timer  = 0.f;
    m_pulse  = 0.f;
    m_phase  = Phase::Result;
    m_cursor = 0;
    m_offered.clear();

    if (!m_cleared) {
        // Clear node on map (not for endless — nodeId==-1)
        if (!m_endless && ctx.nodeMap) {
            ctx.nodeMap->clearNode(m_nodeId);
        }
        // Generate thematic credit bonuses once
        std::mt19937 rng(std::random_device{}());
        generateBonuses(ctx, rng);
        m_cleared = true;
    }
}

void NodeCompleteScene::onExit() {}

// ---------------------------------------------------------------------------
// Offer pool — mix of programs (active) and mods (passive/stat)
// ---------------------------------------------------------------------------
static int programRarityWeight(ProgramRarity r) {
    switch (r) {
        case ProgramRarity::Common:   return 40;
        case ProgramRarity::Uncommon: return 18;
        case ProgramRarity::Rare:     return 6;
    }
    return 1;
}

static int modRarityWeight(ModRarity r) {
    switch (r) {
        case ModRarity::Common:    return 50;
        case ModRarity::Uncommon:  return 20;
        case ModRarity::Rare:      return 7;
        case ModRarity::Legendary: return 2;
    }
    return 1;
}

void NodeCompleteScene::buildOfferPool(SceneContext& ctx) {
    // EXTRA_OFFER shop upgrade: +1 card per stack (max 5 total)
    m_cardCount = CARDS;
    if (ctx.saveData) {
        m_cardCount = std::min(CARDS + ctx.saveData->purchaseCount("EXTRA_OFFER"), 5);
    }

    struct Candidate { UpgradeCard card; int weight; };
    std::vector<Candidate> pool;

    const bool progSlotsAvail    = ctx.programs && !ctx.programs->full();
    const bool passiveSlotsAvail = ctx.mods && !ctx.mods->passiveFull();

    // Add available active programs
    if (progSlotsAvail && ctx.programs) {
        for (int i = 0; i < static_cast<int>(ProgramID::COUNT); ++i) {
            ProgramID pid = static_cast<ProgramID>(i);
            if (ctx.programs->has(pid)) continue;
            const ProgramDef& def = getProgramDef(pid);
            UpgradeCard card;
            card.kind   = UpgradeCard::Kind::Program;
            card.progId = pid;
            pool.push_back({ card, programRarityWeight(def.rarity) });
        }
    }

    // Add available mods
    if (ctx.mods) {
        const int total = static_cast<int>(ModID::COUNT);
        for (int i = 0; i < total; ++i) {
            ModID id = static_cast<ModID>(i);
            const ModDef& def = getModDef(id);
            // Passive mods need a free passive slot
            if (def.type == ModType::Passive && !passiveSlotsAvail) continue;
            // Non-stackable already owned → skip
            if (!def.stackable && ctx.mods->has(id)) continue;
            UpgradeCard card;
            card.kind  = UpgradeCard::Kind::Mod;
            card.modId = id;
            pool.push_back({ card, modRarityWeight(def.rarity) });
        }
    }

    if (pool.empty()) return;

    std::mt19937 rng(std::random_device{}());
    m_offered.clear();
    m_offered.reserve(m_cardCount);

    while ((int)m_offered.size() < m_cardCount && !pool.empty()) {
        int totalW = 0;
        for (auto& c : pool) totalW += c.weight;
        std::uniform_int_distribution<int> dist(0, totalW - 1);
        int roll = dist(rng), acc = 0, chosen = 0;
        for (int j = 0; j < (int)pool.size(); ++j) {
            acc += pool[j].weight;
            if (roll < acc) { chosen = j; break; }
        }
        m_offered.push_back(pool[chosen].card);
        pool.erase(pool.begin() + chosen);
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void NodeCompleteScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (m_phase == Phase::Result) {
        if (m_timer < 1.5f) return;
        bool advance = (ev.type == SDL_KEYDOWN || ev.type == SDL_MOUSEBUTTONDOWN);
        if (!advance && ev.type == SDL_CONTROLLERBUTTONDOWN)
            advance = (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A
                    || ev.cbutton.button == SDL_CONTROLLER_BUTTON_START);
        if (advance) transitionToPick(ctx);
    } else {
        auto handleDir = [&](int d) {
            m_cursor = (m_cursor + d + (int)m_offered.size()) % (int)m_offered.size();
        };
        if (ev.type == SDL_KEYDOWN) {
            const auto sc = ev.key.keysym.scancode;
            if (sc == SDL_SCANCODE_LEFT  || sc == SDL_SCANCODE_A) handleDir(-1);
            if (sc == SDL_SCANCODE_RIGHT || sc == SDL_SCANCODE_D) handleDir(+1);
            if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) pickUpgrade(ctx);
        } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
            const auto btn = ev.cbutton.button;
            if (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT)  handleDir(-1);
            if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) handleDir(+1);
            if (btn == SDL_CONTROLLER_BUTTON_A || btn == SDL_CONTROLLER_BUTTON_START) pickUpgrade(ctx);
        } else if (ev.type == SDL_MOUSEMOTION ||
                   (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT)) {
            int mx = (ev.type == SDL_MOUSEMOTION) ? ev.motion.x : ev.button.x;
            int my = (ev.type == SDL_MOUSEMOTION) ? ev.motion.y : ev.button.y;
            int n   = (int)m_offered.size();
            int cw, cp, x0;
            cardLayout(n, cw, cp, x0);
            for (int i = 0; i < n; ++i) {
                int cx = x0 + i * (cw + cp);
                if (mx >= cx && mx < cx + cw && my >= CARD_Y0 && my < CARD_Y0 + CARD_H) {
                    m_cursor = i;
                    if (ev.type == SDL_MOUSEBUTTONDOWN) pickUpgrade(ctx);
                    break;
                }
            }
        }
    }
}

void NodeCompleteScene::transitionToPick(SceneContext& ctx) {
    buildOfferPool(ctx);
    if (m_offered.empty()) { returnToMap(ctx); return; }
    m_phase  = Phase::UpgradePick;
    m_cursor = 0;
    m_timer  = 0.f;
}

void NodeCompleteScene::pickUpgrade(SceneContext& ctx) {
    if (m_cursor < 0 || m_cursor >= (int)m_offered.size()) { returnToMap(ctx); return; }
    const UpgradeCard& card = m_offered[m_cursor];
    if (card.kind == UpgradeCard::Kind::Program && ctx.programs)
        ctx.programs->add(card.progId);
    else if (card.kind == UpgradeCard::Kind::Mod && ctx.mods)
        ctx.mods->add(card.modId);
    returnToMap(ctx);
}

void NodeCompleteScene::returnToMap(SceneContext& ctx) {
    if (m_endless) {
        // Return to a new endless CombatScene with incremented tier/wave
        NodeConfig cfg;
        cfg.nodeId        = -1;
        cfg.tier          = std::min(4, 1 + ctx.endlessWave / 5);
        cfg.objective     = NodeObjective::Sweep;
        cfg.sweepTarget   = 999999;
        cfg.traceTickRate = 0.17f;
        cfg.endless       = true;
        cfg.endlessWave   = ctx.endlessWave;
        cfg.startTrace    = m_carryTrace;
        ctx.scenes->replace(std::make_unique<CombatScene>(cfg));
    } else {
        if (ctx.nodeMap) ctx.scenes->replace(std::make_unique<MapScene>(*ctx.nodeMap));
        else             *ctx.running = false;
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------
void NodeCompleteScene::update(float dt, SceneContext&) {
    m_timer += dt;
    m_pulse += dt * 3.f;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
void NodeCompleteScene::render(SceneContext& ctx) {
    if (m_phase == Phase::Result) renderResult(ctx);
    else                          renderPick(ctx);
}

void NodeCompleteScene::renderResult(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;

    vr->clear(); vr->drawGrid();

    float progress = std::min(m_timer / 0.8f, 1.f);
    int   lineX    = (int)(progress * Constants::SCREEN_W);
    if (lineX > 1) {
        Vec2 a = { 0.f, Constants::SCREEN_HF * 0.5f };
        Vec2 b = { (float)lineX, Constants::SCREEN_HF * 0.5f };
        vr->drawGlowLine(a, b, {0,220,100});
    }
    vr->drawCRTOverlay();

    // Entrance flash: bright cyan burst that fades in first 0.35s
    if (m_timer < 0.35f) {
        float t = 1.f - m_timer / 0.35f;
        Uint8 a = (Uint8)(t * t * 180.f);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 220, 160, a);
        SDL_Rect full = { 0, 0, Constants::SCREEN_W, Constants::SCREEN_H };
        SDL_RenderFillRect(r, &full);
    }

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int bx = Constants::SCREEN_W/2 - 240, by = Constants::SCREEN_H/2 - 105;
    SDL_SetRenderDrawColor(r, 0, 25, 15, 215);
    SDL_Rect bg = { bx, by, 480, 210 };
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 0, 200, 100, 240); SDL_RenderDrawRect(r, &bg);
    SDL_SetRenderDrawColor(r, 0, 100, 50, 120);
    SDL_RenderDrawLine(r, bx+12, by+140, bx+468, by+140);

    ctx.hud->drawLabel("EXTRACTION COMPLETE", bx+16, by+12,  {0,220,100,255});
    ctx.hud->drawLabel("NODE SECURED",        bx+16, by+40,  {140,200,160,255});
    ctx.hud->drawLabel("ICE ELIMINATED: " + std::to_string(m_iceKilled), bx+16, by+70, {140,200,160,255});
    ctx.hud->drawLabel("SCORE: " + std::to_string(m_score),              bx+16, by+100,{220,220,80,255});

    SDL_Color promptCol = m_timer > 1.5f ? SDL_Color{0,200,160,220} : SDL_Color{60,100,80,140};
    std::string prompt  = m_timer > 1.5f ? ">> PRESS ANY KEY FOR UPGRADE <<" : "...";
    ctx.hud->drawLabel(prompt, bx+16, by+155, promptCol);

    SDL_RenderPresent(r);
}

void NodeCompleteScene::renderPick(SceneContext& ctx) {
    ctx.vrenderer->clear();
    ctx.vrenderer->drawGrid(20,50,80);
    ctx.vrenderer->drawCRTOverlay();

    if (ctx.hud) {
        // Show slot status in header
        int progCount    = ctx.programs ? ctx.programs->count() : 0;
        int passiveCount = ctx.mods     ? ctx.mods->passiveCount() : 0;
        std::string header = "// SYSTEM UPGRADE //";
        std::string slots  = "ACTIVE " + std::to_string(progCount) + "/3   "
                           + "PASSIVE " + std::to_string(passiveCount) + "/3";
        ctx.hud->drawLabel(header, Constants::SCREEN_W/2 - 130, 22, {0,220,180,255});
        ctx.hud->drawLabel(slots,  Constants::SCREEN_W/2 - 110, 52, {80,160,130,200});
    }

    for (int i = 0; i < (int)m_offered.size(); ++i) drawCard(ctx, i);
    SDL_RenderPresent(ctx.renderer);
}

// ---------------------------------------------------------------------------
// Card rendering
// ---------------------------------------------------------------------------

// Returns border color and category/type label for a card
static void cardStyle(const UpgradeCard& card,
                      uint8_t& rr, uint8_t& rg, uint8_t& rb,
                      uint8_t& ar, uint8_t& ag, uint8_t& ab,
                      const char*& typeLabel) {
    if (card.kind == UpgradeCard::Kind::Program) {
        const ProgramDef& def = getProgramDef(card.progId);
        typeLabel = "ACTIVE";
        // Programs: cyan accent
        ar=0; ag=220; ab=200;
        switch (def.rarity) {
            case ProgramRarity::Common:   rr=170; rg=170; rb=170; break;
            case ProgramRarity::Uncommon: rr=40;  rg=200; rb=100; break;
            case ProgramRarity::Rare:     rr=60;  rg=140; rb=255; break;
        }
    } else {
        const ModDef& def = getModDef(card.modId);
        typeLabel = (def.type == ModType::Passive) ? "PASSIVE" : "STAT";
        switch (def.category) {
            case ModCategory::Chassis: ar=40;  ag=210; ab=120; break;
            case ModCategory::Weapon:  ar=230; ag=180; ab=30;  break;
            case ModCategory::Neural:  ar=180; ag=60;  ab=255; break;
            default:                   ar=100; ag=100; ab=100; break;
        }
        rarityColor(def.rarity, rr, rg, rb);
    }
}

void NodeCompleteScene::drawCard(SceneContext& ctx, int idx) const {
    SDL_Renderer* r  = ctx.renderer;
    bool selected    = (idx == m_cursor);
    float pulse      = 0.5f + 0.5f * std::sin(m_pulse);

    // Compute layout dynamically so extra cards (EXTRA_OFFER) stay on screen
    int n   = (int)m_offered.size();
    int cw, cp, x0;
    cardLayout(n, cw, cp, x0);
    int cx  = x0 + idx * (cw + cp);
    int cy  = CARD_Y0;

    const UpgradeCard& card = m_offered[idx];
    uint8_t rr, rg, rb, ar, ag, ab;
    const char* typeLabel;
    cardStyle(card, rr, rg, rb, ar, ag, ab, typeLabel);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Background tinted by rarity
    SDL_SetRenderDrawColor(r, rr/14, rg/14, rb/14, selected ? 220 : 150);
    SDL_Rect cardRect = { cx, cy, cw, CARD_H };
    SDL_RenderFillRect(r, &cardRect);

    // Rarity/kind border
    if (selected) {
        uint8_t alpha = (uint8_t)(180 + 75*pulse);
        SDL_SetRenderDrawColor(r, rr, rg, rb, alpha);
        SDL_RenderDrawRect(r, &cardRect);
        SDL_SetRenderDrawColor(r, rr, rg, rb, (uint8_t)(50*pulse));
        SDL_Rect g1 = {cx-3,cy-3,cw+6,CARD_H+6}; SDL_RenderDrawRect(r,&g1);
        SDL_Rect g2 = {cx-2,cy-2,cw+4,CARD_H+4}; SDL_RenderDrawRect(r,&g2);
    } else {
        SDL_SetRenderDrawColor(r, rr/2, rg/2, rb/2, 180);
        SDL_RenderDrawRect(r, &cardRect);
    }

    // Accent stripe (category/type color)
    SDL_SetRenderDrawColor(r, ar, ag, ab, selected ? 200 : 80);
    SDL_RenderDrawLine(r, cx+4, cy+4, cx+cw-4, cy+4);
    SDL_RenderDrawLine(r, cx+4, cy+5, cx+cw-4, cy+5);

    if (!ctx.hud) return;

    uint8_t nameAlpha = selected ? 255 : 200;
    SDL_Color typeCol   = { ar, ag, ab, nameAlpha };
    SDL_Color nameCol   = { 255, 255, 255, nameAlpha };
    SDL_Color descCol   = { 160, 180, 170, 200 };
    SDL_Color rarityCol = { rr, rg, rb, nameAlpha };

    // Fixed bottom zones: rarity at -48, confirm at -28
    const int rarityY  = cy + CARD_H - 48;
    const int confirmY = cy + CARD_H - 28;

    if (card.kind == UpgradeCard::Kind::Program) {
        const ProgramDef& def = getProgramDef(card.progId);
        std::string cdStr = "CD: " + std::to_string((int)def.cooldown) + "s";
        ctx.hud->drawLabel(typeLabel, cx+10, cy+14, typeCol);
        ctx.hud->drawLabel(def.name,  cx+10, cy+44, nameCol);
        int lines = ctx.hud->drawWrapped(def.desc, cx+10, cy+74, cw - 20, descCol, 20);
        int cdY = std::min(cy + 74 + lines * 20 + 6, rarityY - 22);
        ctx.hud->drawLabel(cdStr, cx+10, cdY, descCol);

        const char* rname = "";
        switch (def.rarity) {
            case ProgramRarity::Common:   rname = "COMMON";   break;
            case ProgramRarity::Uncommon: rname = "UNCOMMON"; break;
            case ProgramRarity::Rare:     rname = "RARE";     break;
        }
        ctx.hud->drawLabel(rname, cx+10, rarityY, rarityCol);
    } else {
        const ModDef& def = getModDef(card.modId);
        ctx.hud->drawLabel(typeLabel,              cx+10, cy+14,    typeCol);
        ctx.hud->drawLabel(def.name,               cx+10, cy+44,    nameCol);
        ctx.hud->drawWrapped(def.desc,             cx+10, cy+74,    cw - 20, descCol, 20);
        ctx.hud->drawLabel(rarityName(def.rarity), cx+10, rarityY,  rarityCol);
    }

    if (selected) {
        SDL_Color hint = { rr, rg, rb, 200 };
        ctx.hud->drawLabel("[ CONFIRM ]", cx+10, confirmY, hint);
    }
}
