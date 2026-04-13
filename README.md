# DEAD SECTOR

A cyberpunk vector-arcade shooter inspired by 1980s arcade games and the aesthetics of William Gibson's *Neuromancer*.
Built with C++17 and SDL2. Runs on Windows and Linux.

Now available on Steam: https://store.steampowered.com/app/4532540/Dead_Sector/
---

## Gameplay

Navigate a **node map** of interconnected cyberspace sectors. Before each node you select three **programs** (abilities with cooldowns) and a starting **hardware mod**. Inside the node you fight ICE — intrusion countermeasure programs — with classic Asteroids-style rotational physics. Your **trace level** climbs continuously; cross thresholds and new ICE types activate. Clear the node objective, collect credits, buy permanent upgrades in the **shop**, and push toward the core.

### ICE types
| ICE | Behavior | Activates at |
|---|---|---|
| Data Construct | Drifts, splits on hit | Always |
| Hunter ICE | Seeks player aggressively | Trace ≥ 25% |
| Sentry ICE | Rotates, fires projectiles | Trace ≥ 50% |
| Spawner ICE | Spawns Hunters continuously | Trace ≥ 75% |
| Pulse Mine | Triggered area denial | Various |
| Boss ICE | Multi-phase objective target | Core nodes |

### Programs (active abilities, cooldown-gated)
| Program | Type | Effect |
|---|---|---|
| FRAG | Offense | 3-way burst shot |
| FEEDBACK | Offense | Radial burst around ship |
| SHIELD | Defense | 2s invincibility |
| EMP | Neural | Stun all ICE for 2s |
| DECRYPT | Neural | Instantly delete nearest ICE |
| STEALTH | Stealth | Halt trace gain for 8s |
| OVERDRIVE | Stealth | Speed boost for 5s |

### Hulls (ships)
| Hull | Unlock | Specialty |
|---|---|---|
| DELTA — Ghost Runner | Always | Balanced starter |
| RAPTOR — Signal Knife | 3 runs | Speed + shot velocity |
| MANTIS — Strike Frame | 50 kills | Thrust + heavy armament |
| BLADE — Ghost Wire | Win a run | Razor precision, tiny hitbox |
| BATTLE — Iron Coffin | 200 kills | Max armor, extra life |

Complete a full run (boss kill) with any hull to gild it — golden hulls show a persistent glow on the selection screen.

---

## Controls

| Input | Action |
|---|---|
| W / A / S / D or Arrow Keys | Navigate menus |
| WASD / Arrow Keys (in combat) | Rotate (A/D) and thrust (W) |
| Space / Enter | Fire / confirm |
| Q / E / R | Programs slot 1 / 2 / 3 |
| ESC (main menu) | Quit |
| ESC (in combat) | Pause |
| Gamepad D-Pad | Navigate |
| Gamepad A | Confirm / fire |
| Gamepad B / Start | Back / pause |
| Gamepad LT/RT | Programs |

---

## Building from Source

### Dependencies

**Linux**
```
sudo apt install cmake libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

**Windows** — cross-compile from Linux with MinGW:
```
sudo apt install cmake g++-mingw-w64-x86-64 mingw-w64
```
Windows SDL2 packages are fetched automatically by the build scripts.

### Build (Linux native)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```
Binary and assets land in `build/`.

### Build (Windows, cross-compile from Linux)
```bash
cmake -S . -B build-windows \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows --parallel
```
This produces `build-windows/dead-sector.exe` with required DLLs.

### Release packages
```bash
./scripts/release.sh v0.3.0
```
Builds both platforms and zips them into `dist/`.

### Tag and publish a GitHub Release
```bash
./scripts/tag-release.sh v0.3.0
```
Commits, tags, pushes — GitHub Actions builds and publishes the release automatically.

---

## Project Structure

```
src/
├── core/           Constants, Programs, Mods, SaveSystem, DisplaySettings
├── entities/       Avatar, ICE types, Projectile, DataConstruct, BossEnemy
├── renderer/       VectorRenderer (glowing vector graphics), HUD, BuildChart
├── scenes/         All game screens (MainMenu, Map, Loadout, Combat, Shop, …)
├── systems/        AI, Collision, Mod, Program, Physics, Spawn, Trace, Particle
├── world/          NodeMap graph
├── audio/          AudioSystem (SDL_mixer wrapper)
├── math/           Vec2
└── debug/          DebugOverlay

assets/
├── fonts/          ShareTechMono-Regular.ttf
└── music/          Background tracks (Karl Casey)

cmake/              MinGW cross-compile toolchain
scripts/            release.sh, tag-release.sh
.github/workflows/  CI release pipeline
```

---

## Display Settings

Configurable from the in-game Settings menu (Main Menu → Settings):

- **Windowed** — choose from 1280×720, 1600×900, 1920×1080, 2560×1440, 4K, and ultrawide presets
- **Fullscreen** — exclusive mode at chosen resolution
- **Borderless** — fullscreen at native desktop resolution (default)

The game renders at a fixed 1280×720 logical resolution and SDL2 scales it to the display. On ultrawide monitors the play area is pillarboxed.

Settings persist across sessions.

---

## Save File

`dead-sector.sav` is written to the working directory. It stores credits, run stats, unlocked hulls, golden hull records, shop purchases, and display preferences.

---

## License

Source code © 2024–2025. All rights reserved.

Music: Karl Casey — licensed separately. Not included in open-source distribution.

Font: Share Tech Mono — [SIL Open Font License](https://scripts.sil.org/OFL).
