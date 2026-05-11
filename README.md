# OpenChess — Nano RP2040 fix-fork

[![Latest release](https://img.shields.io/github/v/release/semichcsc-byte/Open-Chess?label=firmware&color=brightgreen)](https://github.com/semichcsc-byte/Open-Chess/releases/latest)
[![Self-tests](https://img.shields.io/badge/self--tests-10%2F10-brightgreen)](#-self-tests)
[![Sketch size](https://img.shields.io/badge/sketch-150K%20%2F%2016MB%20(0%25)-blue)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](#license)
[![Hardware](https://img.shields.io/badge/hardware-Arduino%20Nano%20RP2040-orange)]()

A bug-fix fork of [Concept-Bytes/Open-Chess](https://github.com/Concept-Bytes/Open-Chess) that **actually works** on the Arduino Nano RP2040 Connect with the Concept-Bytes PCB — same hardware that shipped to [Kickstarter](https://www.kickstarter.com/projects/conceptbytes/open-chess-a-3d-printable-smart-chess-board?ref=cc9981) backers.

> ## 🎯 Are you a Kickstarter backer with a broken AI mode?
>
> **AI mode hangs** at "Connecting to WiFi…"? **Sensors flicker** when you slide pieces? **"Stockfish API failed"** even with internet? **No castling, en passant, or check detection** in the official firmware?
>
> 🔥 **[Download v1.0.0-rp2040 (drag-and-drop `.uf2`)](https://github.com/semichcsc-byte/Open-Chess/releases/latest)** — works in 60 seconds, no Arduino IDE required.
>
> Build documentation, BOM, and full user manual: **[semichcsc-byte/openchess](https://github.com/semichcsc-byte/openchess)**.

---

## Table of contents

- [Quick start (60 seconds)](#-quick-start-60-seconds)
- [What's fixed vs upstream](#-whats-fixed-vs-upstream-concept-bytes)
- [What works now](#-what-works-now)
- [Self-tests](#-self-tests)
- [Compile from source](#-compile-from-source-only-needed-for-ai-mode)
- [Where the hardware comes from](#-where-the-hardware-comes-from)
- [Comparison with other forks](#-comparison-with-other-forks)
- [Upstream PRs](#-upstream-prs)
- [Contributing](CONTRIBUTING.md)
- [Changelog](CHANGELOG.md)
- [User manual](docs/MANUAL.md) — game modes, troubleshooting, WiFi setup
- [License](#license)

---

## ⚡ Quick start (60 seconds)

1. **Download** [`OpenChess-v1.0.0-rp2040.uf2`](https://github.com/semichcsc-byte/Open-Chess/releases/latest) (302 KB).
2. **Double-tap** the white reset button on your Arduino Nano RP2040 Connect within ~500 ms.
3. The board mounts as a USB drive named **`RPI-RP2`**.
4. **Drag** the `.uf2` file onto that drive. The board reboots automatically.
5. Open Serial Monitor at **9600 baud**. You should see:

   ```
   ================================================
            OpenChess Starting Up
            Firmware: v1.0.0-rp2040
            Fork:    semichcsc-byte/Open-Chess
   ================================================
   === ChessEngine self-tests ===
   PASS T1..T10
   === Self-tests complete: 10/10 passed ===
   ```

6. Four white LEDs light up in the centre of the board — place a piece on one to pick a mode. Full manual: **[docs/MANUAL.md](docs/MANUAL.md)**.

> ⚠️ **AI mode requires WiFi credentials.** The pre-built `.uf2` does **not** include them. To use AI mode, [compile from source](#-compile-from-source-only-needed-for-ai-mode). Human-vs-Human and Sensor Test work out of the box.

---

## 🩹 What's fixed vs upstream Concept-Bytes

| Bug / missing in upstream | Fixed here |
|---|---|
| 🔴 AI mode hangs at "Connecting to WiFi…" | ✅ AP→STA tear-down before `WiFi.begin()` ([PR #9](https://github.com/Concept-Bytes/Open-Chess/pull/9)) |
| 🔴 "API request was not successful" on success | ✅ Parser splits HTTP body from headers ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)) |
| 🔴 Easy and Medium AI are identical (same depth) | ✅ `medium()` now sends depth=10 ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)) |
| 🔴 Bot move applied without validation | ✅ Local engine rejects illegal API responses ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)) |
| 🔴 MODE_GAME_3 spam loop forever | ✅ Wait for piece-lift before re-arming menu ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)) |
| 🔴 No check / checkmate / stalemate detection | ✅ Full detection + on-board animation ([PR #11](https://github.com/Concept-Bytes/Open-Chess/pull/11)) |
| 🔴 Pinned pieces could move (expose own king) | ✅ Legal-move filter |
| 🔴 No castling | ✅ Kingside + queenside, FIDE legal |
| 🔴 No en-passant | ✅ Pink LED hint, correct capture removal |
| 🔴 No 50-move rule / insufficient material | ✅ Auto-detected, draw animation |
| 🔴 Pawn always auto-promoted to Queen | ✅ Q/R/B/N choice via 4 LEDs (Human-vs-Human) |
| 🟡 Sensor flicker when sliding pieces | ✅ Debounce: 3 consecutive scans required |
| 🟡 `OpenChessBoard` AP stays up forever (~100 mA) | ✅ Shut down when not needed |
| 🟡 No way to know if firmware is broken | ✅ 10 self-tests at every boot, red flash on failure |

→ **All upstream**, see [PRs #9, #10, #11](#-upstream-prs).

---

## 🎮 What works now

### Game modes
- 👥 **Human vs Human** — castling, en passant, promotion choice (Q/R/B/N via 4 LEDs), check / checkmate / stalemate / 50-move / insufficient material
- 🤖 **Human vs AI (Stockfish)** — Easy/Medium/Hard/Expert via [stockfish.online](https://stockfish.online), bot moves locally validated before applying
- 🔍 **Sensor Test** — visual diagnostics for hall sensors and LED matrix

### UX
- 💡 **Sensor debounce** — slide pieces without false detections
- 🔋 **AP shutdown** when not needed (saves ~100 mA)
- 🆔 **Versioned boot banner** (`v1.0.0-rp2040`) on serial
- ✅ **10 self-tests** before WiFi setup, with red-flash failure indicator

---

## ✅ Self-tests

Every boot runs 10 deterministic chess engine tests and prints:

```
=== ChessEngine self-tests ===
PASS T1: e2 pawn has 2 legal moves
PASS T2: b1 knight has 2 legal moves
PASS T3: no check at start
PASS T4: Fool's Mate detected
PASS T5: pinned rook stayed on file
PASS T6: both castlings legal
PASS T7: no castling in check
PASS T8: en-passant offered
PASS T9: K vs K is draw
PASS T10: kingside castle layout correct
=== Self-tests complete: 10/10 passed ===
```

If any fail, the board flashes red 5 times across all 64 LEDs. **Do not play a game with a broken engine** — re-flash a clean release.

These tests caught a real bug during development (a wrong test fixture) and will catch any regression in the rule engine before it reaches a game.

---

## 🛠️ Compile from source (only needed for AI mode)

### One-time setup

```bash
# Install board support and libraries
arduino-cli core install arduino:mbed_nano
arduino-cli lib install "Adafruit NeoPixel"@1.14.0
arduino-cli lib install WiFiNINA

# Clone this fork
git clone https://github.com/semichcsc-byte/Open-Chess.git
cd Open-Chess
git checkout v1.0.0-rp2040

# Configure WiFi (skip for Human-vs-Human only)
cp arduino_secrets_template.h arduino_secrets.h
# edit arduino_secrets.h with your SSID and password
```

### Compile + upload

```bash
arduino-cli compile --fqbn arduino:mbed_nano:nanorp2040connect .

# find your port:
arduino-cli board list

# replace usbmodemXXX with yours:
arduino-cli upload --fqbn arduino:mbed_nano:nanorp2040connect -p /dev/cu.usbmodemXXX .
```

> 🔒 `arduino_secrets.h` is in `.gitignore` — your WiFi password stays local.

---

## 📦 Where the hardware comes from

| Part | Source |
|---|---|
| **PCB** | [concept-bytes.com/products/openchess-pcb](https://concept-bytes.com/products/openchess-pcb) — originally [Kickstarter campaign](https://www.kickstarter.com/projects/conceptbytes/open-chess-a-3d-printable-smart-chess-board?ref=cc9981) |
| **3D files** (board, tiles, pieces) | [MakerWorld — OpenChess Smart Chess Board](https://makerworld.com/en/models/1256302-openchess-smart-chess-board?from=search#profileId-1279762) by [Concept_Bytes](https://makerworld.com/en/@profileId-1279762) (CC BY 4.0) |
| **Microcontroller** | Arduino Nano RP2040 Connect |
| **Magnets** | 10×2 mm neodymium (32 pieces, south pole down for A3144 sensors) |
| **Steel discs** | 10×1 mm (64, one under each square) |

Full BOM with prices and Amazon links: [openchess/docs/BOM.md](https://github.com/semichcsc-byte/openchess/blob/main/docs/BOM.md) (build documentation repo).

---

## 🔍 Comparison with other forks

| | Concept-Bytes (upstream) | **This fork** | [joojoooo](https://github.com/joojoooo/OpenChess) |
|---|---|---|---|
| **Status** | 🔴 Abandoned (last commit Aug 2025) | 🟢 Active | 🟢 Active (16★) |
| **Hardware** | Nano RP2040 | Nano RP2040 (drop-in) | **ESP32** (jumpers required) |
| **Chess rules** | Pseudo-legal only | Full FIDE | Full FIDE + 3-fold rep |
| **AI mode** | ❌ broken | ✅ Stockfish.online | ✅ Stockfish.online |
| **Lichess online** | ❌ | ❌ (WiFiNINA limits) | ✅ |
| **Web UI** | ❌ | ❌ | ✅ |
| **OTA updates** | ❌ | ❌ | ✅ |
| **Self-tests** | ❌ | ✅ 10 at boot | ❌ |

> If you have an ESP32 and don't mind re-soldering with jumper wires, **[joojoooo/OpenChess](https://github.com/joojoooo/OpenChess) is the more powerful firmware**. This fork exists because the Concept-Bytes Kickstarter campaign shipped the **Nano RP2040 Connect** — the official firmware for that exact MCU has been broken and unmaintained since Aug 2025, leaving backers without working AI mode.

→ Honest feature matrix: **[docs/COMPARISON.md](docs/COMPARISON.md)**.

---

## 📤 Upstream PRs

All three patches are filed against `Concept-Bytes/Open-Chess`. The repo has 0 PRs ever merged so they're unlikely to be reviewed, but at least they're searchable:

- **[PR #9](https://github.com/Concept-Bytes/Open-Chess/pull/9)** — WiFi AP→STA fix (closes #5)
- **[PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)** — 5 quality fixes (depth, parser, validation, MODE_GAME_3, max moves)
- **[PR #11](https://github.com/Concept-Bytes/Open-Chess/pull/11)** — Full chess rules + UX polish + 10 self-tests

If you want the patches in a single branch, use [`feat/rp2040-rules-and-ux`](https://github.com/semichcsc-byte/Open-Chess/tree/feat/rp2040-rules-and-ux) (it's the basis for the v1.0.0-rp2040 tag).

---

## 🤝 Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Bug reports and PRs welcome — please include hardware details, firmware version (printed on serial banner), and full serial output at 9600 baud from boot to the failure.

For build documentation issues (BOM, photos, instructions), open at [`semichcsc-byte/openchess`](https://github.com/semichcsc-byte/openchess/issues) instead.

---

## 🎯 Roadmap

In priority order, all RP2040-friendly:

1. **5-char API move parsing** so AI mode supports promotion choice (`e7e8q`)
2. **Difficulty selection at runtime** (no recompile)
3. **Brightness control** via EEPROM emulation
4. **Async LED animations** (state-machine refactor)
5. **3-fold repetition** if RAM allows

Out of scope (require ESP32): OTA updates, Web UI, Lichess. For these features, see [joojoooo/OpenChess](https://github.com/joojoooo/OpenChess).

---

## License

MIT — same as upstream [Concept-Bytes/Open-Chess](https://github.com/Concept-Bytes/Open-Chess). The OpenChess PCB design and STL files are by Concept Bytes under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
