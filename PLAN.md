# Dead Sector — Development Plan

Current version: **v0.2.6**
Target: Steam Early Access release

---

## Steam Release Requirements

### Platform / Technical
- [ ] **Register real Steam AppID** — replace `480` in `steam_appid.txt`; update `SteamManager` leaderboard names from TODO comments to actual board names created in Steamworks dashboard
- [ ] **Achievements** — define in Steamworks dashboard; wire into `SteamManager` (first boss kill, clear world 1, reach 75% trace, endless wave milestones, golden hull unlock)
- [ ] **Steam Cloud saves** — redirect `dead-sector.sav` through `ISteamRemoteStorage`
- [ ] **Steam Overlay** — handle `GameOverlayActivated_t` callback to pause the game when overlay opens
- [ ] **Steam Input / controller manifest** — register action manifest for full Steam Input support (replaces raw SDL controller queries); enables Steam's own button remapping UI
- [ ] **Add Steamworks SDK to CI** — `release.yml` needs `USE_STEAM=ON` and the SDK available (git-LFS submodule or download step)
- [ ] **Store page** — capsule art, screenshots, trailer, description, tags, system requirements
- [ ] **Windows icon** — embed `.ico` via `.rc` resource file in CMakeLists
- [ ] **Code signing** — Windows Authenticode signing
- [ ] **ESRB / PEGI rating** — complete questionnaire (likely E10+ / PEGI 7)

### Legal
- [ ] Confirm Karl Casey music licensing for commercial distribution; replace or license before release
- [ ] Add credits scene (font: Share Tech Mono OFL, music, engine, tools)

---

## Content

### Node Variety
- [ ] **Infiltrate** objective — reach a data core without detection (trace gates kill instantly)
- [ ] World 3 (The Core) node variety — currently lighter than worlds 0–1
- [ ] Expand events from 12 to ~20 (add ~8 more cyberpunk-flavored buff/debuff events)
- [ ] Lore blurbs on MapScene node tooltips — brief flavour text on hover

### Programs
| Program | Effect |
|---|---|
| OVERCLOCK | Halves all cooldowns for 6s |
| BLACKOUT | Clears all on-screen projectiles |

### Mods
- [ ] **SCATTER CORE** — every 4th shot splits into 3 at impact
- [ ] **DEADMAN SWITCH** — on death, explode dealing 200px AoE
- [ ] **SIGNAL JAM** — Sentries have 40% reduced fire rate

---

## Gameplay

### Balance
- [ ] Boss phase 2 tuning — Archon shield regen, Vortex head fire rate at high waves
- [ ] Endless mode early ramp — difficulty curve gets brutal too fast before wave 10

### Meta Progression
- [ ] **Persistent skill tree** — spend credits outside the shop on passive run-start buffs
- [ ] **Run history** — last 10 runs: hull, score, nodes cleared, cause of death
- [ ] **Daily challenge** — seeded run with fixed hull and mod draft order

### QoL
- [ ] **Tutorial node** — introductory combat with guided prompts; skippable
- [ ] **Program selection persistence** — decide: reset per-node (deck-building) or keep persistent (pick once). If per-node, call `ctx.programs->reset()` in MapScene before LoadoutScene
- [ ] **Save slot selection** — multiple save files

---

## Polish & Juice

*(All major items complete — section removed)*

---

## Technical

- [ ] **CombatScene.cpp** is ~1200 lines — split into CombatInput / CombatRender / CombatLogic files
- [ ] **Version auto-injection** — pass `PROJECT_VERSION` from CMakeLists into `Constants::VERSION` via `configure_file`
- [ ] **macOS build** — `toolchain-macos.cmake` + `macos-latest` CI job in `release.yml`
- [ ] **Linux AppImage** — bundle shared libs for itch.io distribution
- [ ] **Windows installer** — Inno Setup `.exe` for itch.io (Steam uses raw depot files)
- [ ] **Automated tests** — unit tests for SaveSystem, ModSystem, TraceSystem

---

## Release Milestones

| Milestone | Focus |
|---|---|
| **v0.3** | Boss tuning, screen-shake, run history, tutorial |
| **v0.4** | Infiltrate objective, new programs/mods, expanded events |
| **v0.5** | Steam SDK live, achievements, cloud save, store page |
| **v1.0 EA** | Steam release |
