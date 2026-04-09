# Dead Sector — Development Plan

Current version: **v0.3.14**
Target: Steam release (full, not EA) at **$4.99**

---

## Steam Release Checklist

### Steamworks Setup
- [ ] **Register real Steam AppID** — replace `480` in `steam_appid.txt`
- [ ] **Create leaderboards** in dashboard: `Dead_Sector_Normal` and `Dead_Sector_Endless`
- [ ] **Define all 22 achievements** — API names in `SteamManager.hpp`; register each in partner dashboard
- [ ] **Steam Cloud** — quota set (1 MB / 1 file); developer-only mode until tested; uncheck before release
- [ ] **Dynamic Cloud Sync** — leave OFF (game does not implement suspend/resume APIs)
- [ ] **Save file path** — currently saves to working directory (`dead-sector.sav`); should redirect to `SDL_GetPrefPath` before ship (avoids write permission issues on Linux/Steam Deck)

### Store Page
- [x] **Graphical assets** — all 9 Steam images generated (`scripts/gen_steam_assets.py`): logo, header capsule, small capsule, main capsule, vertical capsule, page background, library capsule, library header, library hero
- [x] **Short description** written
- [x] **About This Game** written (BBCode)
- [ ] **Screenshots** — capture in-game (combat, node map, boss fight, loadout, shop)
- [ ] **Trailer** — not yet recorded
- [ ] **Tags** — Roguelike, Shoot 'Em Up, Arcade, Cyberpunk, Action, Indie, 2D, Single Player, Procedural Generation, Sci-fi (min 5 required)
- [ ] **Genres** — Primary: Action; Secondary: Indie
- [x] **System requirements** filled in (Windows + Linux/SteamOS; no macOS)
- [x] **Supported languages** — English only
- [x] **Players** — Single-player only
- [x] **Features** — Steam Achievements, Steam Cloud, Steam Leaderboards, Stats; Full controller support
- [ ] **Price** — $4.99 USD; set per-region pricing in Steamworks Pricing tab
- [ ] **Controller wizard** — complete in Steamworks (SDL GameController, not Steam Input API)

### Legal / Cert
- [ ] **Code signing** — Windows Authenticode
- [ ] **ESRB / PEGI rating** — complete questionnaire (likely E10+ / PEGI 7)
- [ ] **Music licensing** — verify Karl Casey (White Bat Audio) license covers commercial Steam release

---

## Bug Fixes (pre-launch)

### Fixed ✓
- [x] **Vortex boss orbit radius** — was (75, 60), now (190, 170); safe initial distance from player
- [x] **Vortex head fire timers** — were `{0, 0.9, 1.8, 2.7}` (head 3 fired at t=0.3s, ~1s to player death); now `{0, 0.25, 0.5, 0.75}` (first shot no earlier than t=2.25s)
- [x] **RunSummaryScene overflow** — panelH 340→440; prompt anchored at fixed position; hull unlock cap (2 shown); HIGH SCORE/TOTAL CREDITS moved below panel
- [x] **GameOverScene prompt centering** — was hardcoded `-100` offset; now uses `measureText()`

### Known / Remaining
- [ ] **Save file write location** — see Steam Cloud note above
- [ ] **Endless mode ramp** — difficulty curve too steep before wave 10
- [ ] **Archon phase 2 shield regen** — may be too fast; needs balance QA pass

### Fixed (pre-release) ✓
- [x] **Upgrade cards overflow screen** — with EXTRA_OFFER (4-5 cards), total width exceeded 1280px; cards now scale down dynamically to fit

---

## Content

### Implemented ✓
- 3 worlds (SECTOR ALPHA, DEEP NET, THE CORE), 38 total nodes
- 6 ICE enemy types, 3 bosses with phase 2
- 15 programs, 25 mods, 5 ship hulls
- 12 events, endless mode, meta-shop (9 items)
- 22 Steam achievements wired in code

### Remaining
- [ ] **SCATTER CORE mod** — every 4th shot splits into 3 at impact
- [ ] **SIGNAL JAM mod** — Sentries 40% reduced fire rate
- [ ] **World 3 node variety** — The Core is lighter than worlds 1–2
- [ ] **Expand events** — currently 12; target 18–20

---

## Release Milestones

| Milestone | Focus |
|---|---|
| **v0.4** | SCATTER CORE / SIGNAL JAM, expanded events, save path fix |
| **v0.5** | Real AppID live, all store assets, screenshots, trailer |
| **v1.0** | Steam release |
