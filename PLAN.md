# Dead Sector — Development Plan

Current version: **v0.3.9**
Target: Steam Early Access release

---

## Steam Release Requirements

- [ ] **Register real Steam AppID** — replace `480` in `steam_appid.txt`; create leaderboards `Dead_Sector_Normal` and `Dead_Sector_Endless` in Steamworks dashboard
- [ ] **Define achievements in Steamworks dashboard** — 20 achievements coded and wired; register all API names from `SteamManager.hpp`
- [ ] **Store page** — capsule art, screenshots, trailer, description, tags, system requirements
- [ ] **Code signing** — Windows Authenticode signing
- [ ] **ESRB / PEGI rating** — complete questionnaire (likely E10+ / PEGI 7)

---

## Content

### Node Variety
- [ ] **World 3 node variety** — The Core is lighter than worlds 1–2; add more distinct node types
- [ ] **Expand events** — currently 12; add more cyberpunk-flavored buff/debuff events
- [ ] **Lore blurbs** — brief flavour text on MapScene node tooltip hover

### Programs
15 programs implemented: FRAG, EMP, STEALTH, SHIELD, OVERDRIVE, DECRYPT, FEEDBACK, CLONE, OVERCLOCK, BLACKOUT, BREACH, TWIN SHOT, BEACON, NOVA RING, GRAV WELL.

### Mods
- [ ] **SCATTER CORE** — every 4th shot splits into 3 at impact
- [ ] **SIGNAL JAM** — Sentries have 40% reduced fire rate

---

## Balance
- [ ] Boss phase 2 tuning — Archon shield regen rate, Vortex head fire rate at high waves
- [ ] Endless mode early ramp — difficulty curve too steep before wave 10

---

---

## Release Milestones

| Milestone | Focus |
|---|---|
| **v0.4** | SCATTER CORE / SIGNAL JAM, expanded events, World 3 node variety |
| **v0.5** | Real AppID live, store page, World 3 variety |
| **v1.0 EA** | Steam release |
