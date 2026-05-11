# Honest comparison: this fork vs joojoooo/OpenChess vs upstream Concept-Bytes

> ⚖️ This document exists so you can pick the right firmware **for your hardware**, without me overselling my own fork. Each fork is good at different things.

## TL;DR

| You have… | Use… |
|---|---|
| **Concept-Bytes PCB + Arduino Nano RP2040 Connect** (the Kickstarter / official kit) | 🟢 [`semichcsc-byte/Open-Chess`](https://github.com/semichcsc-byte/Open-Chess) (this fork) — drop-in `.uf2`, no re-soldering |
| **Concept-Bytes PCB + Arduino Nano ESP32** (you swapped the MCU) | 🟢 [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess) — far more features, requires jumper wires (PCB pinout doesn't match ESP32 directly) |
| **Custom hardware / from-scratch build** | 🟢 [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess) — auto-calibration figures out your pin order |

The two forks **target different MCUs** and are not interchangeable on the same board without hardware changes.

---

## Detailed feature matrix

Status as of **May 2026**. ✅ = implemented and working, ⚠️ = partial or with caveats, ❌ = not present.

### Chess rules

| Feature | Concept-Bytes (upstream) | semichcsc-byte (this) | joojoooo |
|---|---|---|---|
| Pseudo-legal move generation | ✅ | ✅ | ✅ |
| Legal-move filtering (own-king-check) | ❌ pinned pieces could expose king | ✅ | ✅ |
| Check detection + display | ❌ | ✅ red king blink | ✅ blinking animation |
| Checkmate detection + display | ❌ game never ends | ✅ winner-coloured animation | ✅ |
| Stalemate detection | ❌ | ✅ fireworks | ✅ |
| 50-move rule | ❌ | ✅ | ✅ |
| 3-fold repetition | ❌ | ❌ (RAM-heavy) | ✅ |
| Insufficient material draw | ❌ | ✅ K-vs-K, K+minor-vs-K | ✅ |
| Castling (kingside + queenside, FIDE) | ❌ | ✅ all rules incl. attacked-path | ✅ |
| En passant | ❌ | ✅ pink LED hint | ✅ purple LED hint |
| Promotion choice (Q/R/B/N) | ❌ always Queen | ✅ via 4 LEDs (Human-vs-Human) | ✅ via WebUI / ChessUp app |
| Promotion in AI mode | ❌ always Q | ⚠️ always Q (5-char API parse pending) | ✅ Q via WebUI |
| Resign / Draw button | ❌ | ❌ | ✅ WebUI button or "lift both kings 2s" |
| Game-history recovery on power loss | ❌ | ❌ | ✅ |
| Bot move validation against local engine | ❌ | ✅ rejects illegal API responses | ✅ |

### Hardware & UX

| Feature | Concept-Bytes | semichcsc-byte | joojoooo |
|---|---|---|---|
| Sensor debounce | ❌ flicker on slide | ✅ 3-scan debounce | ✅ |
| AP shutdown when not needed | ❌ AP always on (~100 mA) | ✅ off in non-bot modes | ✅ on-demand |
| Brightness control | ❌ fixed `BRIGHTNESS=100` | ❌ same as upstream | ✅ adjustable + dark-square contrast |
| LED animations (async, non-blocking) | ❌ all `delay()`-based | ❌ same as upstream | ✅ async |
| Auto-calibration of GPIOs / LED order | ❌ | ❌ | ✅ rotates/flips board automatically |
| Inverted shift register (PNP support) | ❌ | ❌ | ✅ |
| Verbose serial debug | ⚠️ very verbose | ⚠️ same as upstream | ✅ structured |
| On-boot self-tests | ❌ | ✅ 10/10 deterministic | ❌ |
| Boot banner with firmware version | ❌ | ✅ | ✅ |

### WiFi & networking

| Feature | Concept-Bytes | semichcsc-byte | joojoooo |
|---|---|---|---|
| WiFi AP for mode selection | ✅ but conflicts with STA | ✅ + clean teardown before STA | ✅ as captive portal fallback |
| AP→STA transition | ❌ silently hangs (issue #5) | ✅ explicit `WiFi.disconnect()` + `WiFi.end()` | ✅ |
| WiFi credentials baked in code | ⚠️ `arduino_secrets.h` (manual edit + reflash) | ⚠️ same today; ⏳ planned for v1.2 (Web UI) | ✅ Web UI input, 3 saved networks, fast reconnect |
| HTTP body parser robust to header JSON | ❌ grabs first `{` (Cloudflare `Nel:` header) | ✅ splits on `\r\n\r\n` | n/a (uses different lib) |
| Stockfish API integration | ⚠️ broken (parser bug) | ✅ working | ✅ working |
| Lichess online play | ❌ | ⏳ possible via NDJSON streaming, but fragile on WiFiNINA — not on near-term roadmap | ✅ play live games on physical board |
| Chess.com integration | ❌ | ❌ | ⚠️ planned |

### Software infrastructure

| Feature | Concept-Bytes | semichcsc-byte | joojoooo |
|---|---|---|---|
| OTA firmware updates | ❌ | ❌ **won't fix** — RP2040 has no dual flash partition + no `Update.h`; the upstream mbed core tried (`second_stage_ota` patch) and reverted. Use `.uf2` drag-and-drop instead. | ✅ via Web UI drag&drop |
| Auto-update from GitHub releases | ❌ | ❌ (depends on OTA) | ✅ checks at boot |
| Web UI for config / state / move history | ❌ | ⏳ **planned for v1.2.0-rp2040** — LittleFS is available on the mbed core, existing AP/HTTP server can be extended | ✅ full-featured |
| Customisable themes (piece, square, sound) | ❌ | ❌ (not planned) | ✅ |
| Web Flasher (browser-based) | ❌ | ❌ (no WebUSB on RP2040; `.uf2` drag-and-drop is the moral equivalent) | ✅ flash from Chrome via WebUSB |
| GitHub Actions CI/CD | ❌ | ❌ (trivial to add, not done yet) | ✅ tagged builds + release artifacts |
| Self-tests | ❌ | ✅ 10 tests at boot | ❌ |
| Tagged releases with `.uf2`/`.bin`/`.hex` | ❌ | ✅ v1.0.0-rp2040 | ✅ v1.0.6 (latest) |

### Hardware compatibility

| Hardware | Concept-Bytes | semichcsc-byte | joojoooo |
|---|---|---|---|
| **Arduino Nano RP2040 Connect** (kit default) | ✅ (with bugs) | ✅ **target** | ❌ wrong pinout |
| Arduino Nano 33 IoT (WiFiNINA) | ⚠️ mostly | ⚠️ should work, untested | ❌ |
| Arduino MKR WiFi 1010 (WiFiNINA) | ⚠️ mostly | ⚠️ should work, untested | ❌ |
| **Arduino Nano ESP32** | ❌ | ❌ | ✅ **target** (jumpers required) |
| ESP32-WROOM / DevKit | ❌ | ❌ | ✅ |

---

## What this fork has that joojoooo doesn't

(Yes, even joojoooo has some gaps — though small.)

- **On-boot self-tests** — 10 deterministic chess engine tests with PASS/FAIL on the serial monitor and a red-flash failure indicator. Catches engine regressions before they reach a game. joojoooo has more features but no automated correctness guard.
- **No re-soldering required** — runs on the Concept-Bytes PCB pinout out of the box. joojoooo requires jumper wires + sometimes trace cuts.
- **Drop-in `.uf2`** for the official kit — double-tap reset, drag-and-drop, done. joojoooo requires PlatformIO + Web Flasher (or its OTA system).

These are deliberate design choices, not oversights — joojoooo trades simplicity for power.

---

## What joojoooo has that we don't (yet)

A lot. If you have an ESP32, **just use joojoooo** — no point reinventing things that already work better there. For the Nano RP2040, here's what's realistic vs not:

**joojoooo has, we plan to add (v1.2+):**
- **Web UI** — LittleFS is available on the mbed core, the existing AP/HTTP server in `wifi_manager.cpp` can be extended. We'll match the **configuration** features (WiFi, mode, difficulty, FEN export, draw/resign) but not the depth (themes, sounds, evaluation graphs).

**joojoooo has, we won't realistically match:**
- **Lichess online play** — technically possible (see Roadmap), but the right hardware for online play is ESP32. We'd be reinventing the wheel with a less reliable WiFi stack.
- **OTA updates** — ESP32 has dual flash partitions and `Update.h` built-in. RP2040 doesn't, and emulating it is weeks of bootloader work for marginal gain over `.uf2` drag-and-drop.
- **Async architecture** — RP2040 has 2 cores but the WiFiNINA stack is single-thread. We can do `millis()`-based async LED animations (planned v1.1) but not a full async web server.
- **3-fold repetition** — possible if RAM allows (planned v1.1).
- **Resign / Draw buttons** — will come with the v1.2 Web UI.
- **Game-history recovery on power loss** — needs LittleFS + state serialiser, comes naturally with Web UI work.
- **Auto-calibration of GPIOs / LED layout** — high complexity, low value for the Concept-Bytes PCB (pinout is fixed). Not planned.
- **CI/CD with auto-builds** — trivial GitHub Actions setup, will add when the version cadence justifies it.

The deliberate non-goals are clearly marked **[Won't fix](#wont-fix-rp2040-architectural-limits)** in the Roadmap section.

## Why have two forks at all?

Because **the Concept-Bytes Kickstarter campaign shipped Nano RP2040 Connect kits** to backers, but the official firmware for that exact MCU has been broken and unmaintained since Aug 2025.

If you're reading this because your AI mode hangs, you're a Kickstarter backer who paid for working hardware. The patched firmware in this fork is the path of least resistance: same MCU, same pinout, same `.uf2` workflow you already know. No buying new components, no re-soldering.

If you're willing to invest **~€20 + 1 evening** in a Nano ESP32 + jumper wires, joojoooo's fork is undeniably better. But that's a real commitment, and not everyone wants to make it just to play chess against Stockfish.

Both forks treat each other's existence as legitimate. There's no fork war here.

---

## Roadmap

### `semichcsc-byte/Open-Chess` (this fork) — what's next

In priority order, all RP2040-friendly:

**v1.1.0-rp2040** (small, high-impact, ≤1 session each):

1. **5-char API move parsing** so AI mode supports promotion choice (`e7e8q`)
2. **Difficulty selection at runtime** — pick Easy/Medium/Hard/Expert from the 4 selector squares after AI mode (no recompile)
3. **Brightness control** — read non-volatile value (Arduino EEPROM emulation on RP2040)
4. **Async LED animations** — refactor `firework`/`capture`/`promotion` to a `millis()`-based state machine
5. **3-fold repetition** if RAM allows (~2 KB for last 100 positions)

**v1.2.0-rp2040** (medium-effort, very high value):

6. **Web UI** — the LittleFS partition is available on the mbed core and the existing `wifi_manager.cpp` AP/HTTP server can be extended. Realistic deliverables:
    - Configure WiFi without editing `arduino_secrets.h` + recompiling
    - Mode selection from browser
    - Live board state + FEN export
    - Difficulty selection
    - Resign / Draw buttons
    - Move history (in-RAM, lost on reboot)

   Won't match joojoooo's depth (no themes, no move sounds, no evaluation graphs) — just enough to remove the recompile-for-WiFi pain point.

**v1.3.0-rp2040** (speculative, only if there's user demand):

7. **Lichess** integration via NDJSON streaming. Possible: Lichess Bot API uses HTTPS long-poll, not WebSocket, which WiFiNINA tolerates. Hard parts: heap fragmentation over long games, TLS handshake latency on Cloudflare reconnect (~3-5 s gap), token storage. Honest estimate: 3–4 sessions of work + tuning. **Most users wanting Lichess on a physical board are better off with [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess) on an ESP32**, so this is low-priority unless someone files a feature request.

### Won't fix (RP2040 architectural limits)

- **OTA firmware updates** — the RP2040 has no dual flash partition (A/B), no `Update.h` library, and the upstream mbed core attempted a `second_stage_ota` patch and reverted it. A custom bootloader is theoretically possible but has weeks of development cost and a real risk of bricking user boards. The `.uf2` drag-and-drop workflow we already have is fine: double-tap reset → drop file → done in 5 seconds.
- **Web Flasher** — no WebUSB driver path exists for the RP2040. The `.uf2` workflow is the moral equivalent.
- **Lichess WebSocket** transport — not used by Lichess Bot API anyway (they use NDJSON over HTTPS).

If you need any of the above, **use [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess) on an ESP32**. It's the right project for that hardware and that feature set.

### `joojoooo/OpenChess` — what's coming

I don't speak for that project. Watch their [GitHub](https://github.com/joojoooo/OpenChess) for updates.

---

## Maintainer notes

If you're considering forking either project: please don't fork to fix one bug and abandon. Both maintainers will gladly merge a clean PR.

- **For RP2040 issues:** [open an issue here](https://github.com/semichcsc-byte/Open-Chess/issues) or submit a PR.
- **For ESP32 issues:** [open an issue at joojoooo/OpenChess](https://github.com/joojoooo/OpenChess/issues).
- **For the abandoned upstream:** PRs #9, #10, #11 are open at [Concept-Bytes/Open-Chess](https://github.com/Concept-Bytes/Open-Chess/pulls). They probably won't be reviewed, but at least they're searchable for future visitors hitting the same bugs.
