# Dead Sector — Development Plan

Current version: **v0.2.6**
Target: Steam Early Access release

---

## Steam Release Requirements

### Platform / Technical
- [ ] **Register real Steam AppID** — replace `480` in `steam_appid.txt`; create leaderboards "Dead_Sector_Normal" and "Dead_Sector_Endless" in Steamworks dashboard
- [ ] **Define achievements in Steamworks dashboard** — 7 achievements coded and ready: `ACH_FIRST_KILL`, `ACH_FIRST_BOSS`, `ACH_CLEAR_WORLD1`, `ACH_CLEAR_WORLD2`, `ACH_FINAL_BREACH`, `ACH_GOLDEN_HULL`, `ACH_HIGH_TRACE`
- [x] **Achievement unlock logic** — wired into CombatScene, VictoryScene
- [x] **Steam Overlay pause** — `GameOverlayActivated_t` → `isOverlayActive()` pauses game loop
- [x] **Steam Input manifest** — `assets/actions/game_actions.vdf` created; upload via Steamworks dashboard
- [x] **Windows icon** — `.rc` resource file added; drop `assets/icon.ico` before Windows build
- [x] **Steam Cloud saves** — `cloudSave`/`cloudLoad` via `ISteamRemoteStorage`; synced in `Game::init` and `Game::shutdown`
- [x] **Add Steamworks SDK to CI** — both workflows download SDK from `STEAMWORKS_SDK_URL` secret and build with `USE_STEAM=ON` when present
- [ ] **Store page** — capsule art, screenshots, trailer, description, tags, system requirements
- [ ] **Code signing** — Windows Authenticode signing
- [ ] **ESRB / PEGI rating** — complete questionnaire (likely E10+ / PEGI 7)

### Legal
- [ ] Confirm Karl Casey music licensing for commercial distribution; replace or license before release
- [x] Credits scene exists (CreditsScene.cpp — scrolling credits with font, music, engine attribution)

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

- [x] **CombatScene.cpp** split — CombatScene.cpp (lifecycle) / CombatSceneUpdate.cpp / CombatSceneRender.cpp
- [x] **Version auto-injection** — `Constants::VERSION` now reads `DEAD_SECTOR_VERSION` compile def set from `PROJECT_VERSION` in CMakeLists
- [x] **macOS build** — `macos-latest` CI job in `build.yml` + `release.yml`; SDL2 via Homebrew; dylibs bundled in package
- [x] **Linux AppImage** — `linuxdeploy` in release CI; `.desktop` file at `installer/dead-sector.desktop`
- [x] **Windows installer** — NSIS script at `installer/dead-sector.nsi`; built via `makensis` in release CI
- [x] **Automated tests** — `ctest` suite: SaveSystem (round-trip + logic), ModSystem (multipliers + flags), TraceSystem (tick, thresholds, callbacks)

---

## Release Milestones

| Milestone | Focus |
|---|---|
| **v0.3** | Boss tuning, run history, tutorial node |
| **v0.4** | Infiltrate objective, new programs/mods, expanded events |
| **v0.5** | Real AppID live, cloud save, CI Steam build, store page |
| **v1.0 EA** | Steam release |
