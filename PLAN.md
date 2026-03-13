# Dead Sector — Development Plan

Current version: **v0.2.2**
Target: Steam Early Access release

---

## Steam Release Requirements

### Platform / Technical
- [ ] **Steam SDK integration** — `steam_api64.dll` / `libsteam_api.so`; replace quit path with `SteamAPI_Shutdown()`
- [ ] **Achievements** — define in Steamworks dashboard; hook into game events (first run, boss kill, golden hull, endless wave milestones)
- [ ] **Steam Cloud saves** — redirect `dead-sector.sav` through `ISteamRemoteStorage`
- [ ] **Steam Overlay** — call `SteamAPI_RunCallbacks()` each frame; overlay requires `SDL_WINDOW_OPENGL` or compositor workaround under SDL2
- [ ] **Steam Controller API** — register action manifest for full Steam Input support (replaces raw SDL controller queries)
- [ ] **Store page** — capsule art, screenshots, trailer, description, tags, system requirements
- [ ] **Windows icon** — embed `.ico` via `.rc` resource file in CMakeLists; set `ProductVersion` in VERSIONINFO
- [ ] **macOS build** — SDL2 is portable; add `toolchain-macos.cmake` and CI step
- [ ] **Code signing** — Windows Authenticode signing; macOS notarization
- [ ] **ESRB / PEGI rating** — complete questionnaire (likely E10+ / PEGI 7 — fantasy violence)

### Legal
- [ ] Confirm music licensing for distribution (currently Karl Casey tracks — verify commercial rights)
- [ ] Replace or license any placeholder audio before release
- [ ] Add credits screen (font: Share Tech Mono OFL, music, engine, tools)

---

## Content — Immediate Priority

### More Node Variety
- [ ] **Infiltrate** objective — reach a data core position without being detected (trace gates kill you)
- [ ] **Survive** objective — outlast a timer with escalating ICE waves (no kill quota)
- [ ] **Extract** objective (partially implemented as Sweep) — collect data packets dropped by ICE
- [ ] Larger node map: 16–20 nodes across 4 tiers (Entry → Mid → Deep → Core)
- [ ] Per-zone color themes: Entry=cyan, Mid=green, Deep=magenta, Core=red/gold

### More Programs
| Program | Type | Effect |
|---|---|---|
| CLONE | Neural | Decoy ship draws ICE attention for 4s |
| OVERCLOCK | Neural | Halves all cooldowns for 6s |
| BLACKOUT | Stealth | Kill all on-screen projectiles |
| BREACH | Offense | Fires a piercing beam across the screen |

### More Mods (hardware upgrades between nodes)
- **SCATTER CORE** (Weapon/Offense) — every 4th shot splits into 3 at impact
- **DEADMAN SWITCH** (Chassis/Defense) — on death, explode dealing 200px AoE
- **SIGNAL JAM** (Neural/Stealth) — Sentries have 40% reduced fire rate
- **OVERLOAD COIL** (Weapon/Offense) — shots deal +50% damage to Spawners

### More ICE Types
- [x] **Phantom ICE** — invisible until within 120px; blinks visible before firing
- [x] **Leech ICE** — attaches to avatar; doesn't damage but accelerates trace gain
- [x] **Mirror ICE** — reflects projectiles back at the player

---

## Gameplay Improvements

### Difficulty & Balance
- [x] **Difficulty selector** on run start (Runner / Decker / Netrunner) — scales trace tick rate, ICE health, spawn intervals
- [x] **Boss phase 2** — boss transitions at 50% health to a second attack pattern
- [ ] **Credit economy tuning** — playtesting data needed; shop prices likely need adjustment
- [x] **Endless mode scaling** — after wave 10, introduce Phantom and Leech ICE; boss every 5 waves

### Meta Progression
- [ ] **Persistent skill tree** — spend credits on passive buffs (starting ammo +5, trace tick -5%, etc.) outside the shop
- [ ] **Run history** — last 10 runs stored: hull, score, nodes cleared, cause of death
- [ ] **Daily challenge** — seeded run with fixed hull and mod draft order

### QoL
- [ ] **Tutorial node** — introductory combat with guided prompts; skippable after first completion
- [ ] **Pause — full build review** — expand pause build chart to show effective stat numbers, not just deltas
- [ ] **Control remapping** — keyboard bindings configurable in Settings
- [ ] **Colorblind mode** — ability type palette alternate (shape-coded as well as color-coded)
- [ ] **Save slot selection** — multiple save files for different players

---

## Technical Debt & Refactoring

### Architecture
- [ ] **Program selection persistence** — LoadoutScene currently only lets you pick programs on node 1; subsequent nodes show the screen but can't add programs (already full). Decide: reset per-node (true deck-building) or keep persistent (pick once per run). If per-node, call `ctx.programs->reset()` in `MapScene` before navigating to `LoadoutScene`.
- [ ] **ESC routing** — currently each scene independently handles ESC; consider a scene stack so `pop()` handles back navigation uniformly
- [ ] **DisplaySettings.hpp** static arrays — harmless but verbose; could move to `.cpp` with `extern const` for cleaner link semantics
- [ ] **CombatScene.cpp** is 1160 lines — consider splitting: `CombatInput`, `CombatRender`, `CombatLogic` into separate files or helper classes

### Rendering
- [ ] **Sprite/texture cache** — currently all rendering is immediate SDL draw calls; a simple texture atlas for common shapes would reduce per-frame draw call overhead
- [ ] **Screen shake polish** — current shake uses raw viewport offset; could blend to zero with a spring damper for a smoother feel
- [ ] **HUD font scaling** — HUD always renders at logical 1280×720; at 4K monitors the text appears small. Consider a second HUD font at 2× size for high-DPI displays (detected via `SDL_GetDisplayDPI`)

### Audio
- [ ] **Sound effects** — currently music only; needs: laser fire, ICE death, threshold alarm, program activation, hit, death
- [ ] **Procedural audio** — generate SFX from SDL audio callbacks (sine/square/noise) to avoid asset licensing issues
- [ ] **Music variety** — at least 3 tracks with cross-fade between map / combat / boss

### Build & CI
- [ ] **macOS GitHub Actions runner** — add `macos-latest` job to `release.yml`
- [ ] **Automated testing** — unit tests for `SaveSystem`, `ModSystem`, `TraceSystem` using Catch2 or doctest
- [ ] **Version auto-injection** — pass `PROJECT_VERSION` from CMakeLists into `Constants::VERSION` via `configure_file` instead of manual sync
- [ ] **Installer** — Inno Setup script for Windows `.exe` installer (Steam depot will use raw files, but useful for itch.io)
- [ ] **Linux AppImage** — bundle shared libs for distribution without package manager dependencies

---

## Polish & Juice

- [ ] **Death animation** — ship explodes into fragments with particle burst; currently just disappears
- [ ] **Node completion fanfare** — brief VFX sequence instead of instant scene transition
- [ ] **Trace threshold flash** — full-screen color pulse + audio sting when crossing 25/50/75/100%
- [ ] **Credits roll** — scrolling credits scene accessible from main menu
- [ ] **High score leaderboard** — local first, optionally Steam Leaderboards
- [ ] **Gamepad rumble** — on hit, boss death, threshold crossing
- [ ] **Localization** — i18n skeleton; target: English, Japanese, German, Brazilian Portuguese
- [ ] **Accessibility** — subtitle-style on-screen event log for audio cues

---

## Release Milestones

| Milestone | Content | Target |
|---|---|---|
| **v0.3.0** | Tutorial, SFX, boss phase 2, difficulty selector | +6 weeks |
| **v0.4.0** | Full 20-node map, 4 new programs, run history | +4 weeks |
| **v0.5.0** | Steam SDK integration, achievements, cloud save | +6 weeks |
| **v1.0 EA** | Steam release, macOS build, store page live | TBD |
