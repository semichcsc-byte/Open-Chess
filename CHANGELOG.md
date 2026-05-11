# Changelog

All user-visible changes to this firmware fork. Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [v1.1.1-rp2040] — 2026-05-11

Patch release: add a non-blocking 'bot thinking' animation that survives the WiFiSSL TLS handshake.

🔗 [Release page](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.1.1-rp2040)

### Added

- **Breathing pulse on rank 8 while the AI is thinking.** A slow blue breathing animation (sine-wave brightness, ~1.5 s period) plays on the bot's back rank from the moment you complete your move until the Stockfish API responds. Lets you see the board is alive instead of just frozen.

### Fixed

- **NeoPixel writes were corrupting the WiFiSSL TLS handshake.** The first attempt at this animation called `clearAllLEDs()` + `showLEDs()` ~20 times per second. Each NeoPixel `show()` disables interrupts for ~2.5 ms (bit-banging the WS2812 protocol), and during the concurrent SSL handshake / read in `WiFiSSLClient`, that was enough to drop TLS records and cause `Failed to connect to Stockfish API` on every move. Fix: animation now only redraws when its discrete brightness bucket actually changes (8 levels over the 1.5 s period → ~5 fps average), and we cleared the strip explicitly between the AI's response and its move display so there's no overlap.

### Verified

```
Sketch uses 148200 bytes (0%) of program storage space.
Global variables use 44640 bytes (16%) of dynamic memory.
=== Self-tests complete: 10/10 passed ===
```

End-to-end test on Arduino Nano RP2040 Connect, WiFiNINA 3.0.1: e2-e4 → bot thinking pulse plays for ~2 s → Stockfish responds with `bestmove e7e5` → board displays the bot's move correctly.

### Known issue (deferred to v1.2)

The serial debug log still prints chess notation that's mirrored across the rank axis (a move from `e2 to e4` is logged as `from e7 to e5`). Internally the firmware tracks the position correctly and Stockfish receives the right FEN — only the human-readable serial print has a row-axis mirror, the same kind of bug that was fixed for columns in v1.1. Will be fixed by aligning `chess_bot.cpp` and `chess_moves.cpp` print helpers with the standard FEN row convention (`row 0 = rank 8`).

---

## [v1.1.0-rp2040] — 2026-05-11

UX overhaul + a long-standing **column-mirror coordinate bug** fixed at the driver layer.

🔗 [Release page with `.uf2` / `.bin` / `.hex` binaries](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.1.0-rp2040)

### Fixed

- **🐛 COORDINATE BUG: columns were mirrored at the hardware layer.** The Concept-Bytes PCB wires the sensor matrix and LED strip such that physical file `a` lands on internal column 7, file `h` on column 0. The original firmware never accounted for this, so every move printed to serial had its file letter mirrored (`a` ↔ `h`, `b` ↔ `g`, etc.). The board worked internally because everything stayed consistent within the firmware, but:
    - Serial debug logs were misleading (`Player moved P from a7 to a5` actually meant `h7-h5`)
    - The Stockfish FEN/move strings were technically wrong (the engine got mirrored positions, but since the position was internally self-consistent, the suggested moves were valid for what the engine saw — just labelled with the wrong file letters)
    - Any future Web UI or PGN export would show the wrong notation
  
  Verified by an interactive 4-corner calibration test (place piece on h1 → reads `(0, 0)` instead of `(0, 7)`, etc.). Fixed at the driver layer in [`board_driver.cpp`](board_driver.cpp): both `readSensors()` and `getPixelIndex()` now flip `col` to `7 - col` so the engine sees standard chess coordinates with `col 0 = file a`. **All 36 callers (engine, chess_moves, chess_bot, animations) untouched** — the fix is fully encapsulated in `BoardDriver`.

  Affects every previous release including v1.0.0-rp2040 and the upstream Concept-Bytes firmware. **Anyone reading their serial monitor previously was reading mirrored notation.**

### Added

- **Setup hint at boot**: as soon as the board powers on, the 16 squares of the white starting position glow soft white and the 16 black squares glow red. Place each piece in its starting square and the corresponding LED goes dark. No more guessing where the pieces go.
- **Convergent rainbow explosion** when all 32 pieces are in place: 4 collapsing rainbow rings (red → orange → yellow → green) sweep from the outer edge to the centre, followed by 4 diagonal beams from the corners (magenta / cyan / amber / lime), then a white shockwave bursts back outwards, ending with a triple pulse on the 2 selector squares. ~3.5 seconds of spectacle.
- **Simplified 2-option mode menu**: D5 lights up for Human-vs-Human, E4 for AI mode (Sensor Test still available in code but removed from the menu — it was confusing to land on by accident).
- **Sticky menu**: once the explosion plays and the menu appears, lifting a piece to use as a selector no longer regresses to the setup hint. Menu stays visible until you actually pick a mode.
- **Reset gesture**: while playing in any chess mode, if you put all 32 pieces back onto their starting squares and hold them there for 1.5 seconds, the firmware drops back to the menu (re-runs the explosion + selector). Lets you finish a game and start a different mode without power-cycling. Will not trigger at game start because the gesture requires the board to have *deviated* from the starting position at least once during the current mode.
- **Boot banner** prints firmware version + fork name on serial.

### Verified

```
Sketch uses 149724 bytes (0%) of program storage space.
Global variables use 44648 bytes (16%) of dynamic memory.
=== Self-tests complete: 10/10 passed ===
```

Tested on Arduino Nano RP2040 Connect with WiFiNINA firmware 3.0.1, Concept-Bytes PCB v1, arduino-cli 1.4.1, arduino:mbed_nano core 4.5.0.

### Migration notes

- **Coordinates change for the user only in the serial monitor.** The on-board LEDs and gameplay are visually identical except now labelled correctly. If you've been reading move logs, the file letters are now correct (a-file on the left when looking from white's side, h-file on the right).
- No `arduino_secrets.h` change required.
- Re-flashing v1.1 over v1.0 is safe; no state migration needed.

---

## [v1.0.0-rp2040] — 2026-05-11

First tagged release of the [`semichcsc-byte/Open-Chess`](https://github.com/semichcsc-byte/Open-Chess) Nano RP2040 fix-fork.

🔗 [Release page with `.uf2` / `.bin` / `.hex` binaries](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.0.0-rp2040)

### Added

- **Full chess rules engine** ([PR #11](https://github.com/Concept-Bytes/Open-Chess/pull/11)):
  - Check, checkmate, stalemate detection with on-board animations
  - Castling (kingside + queenside, FIDE-legal — king-not-in-check, doesn't-pass-through-attacked, rights tracked in `GameState`)
  - En passant (pink LED hint on capture target, captured pawn removed from real square)
  - 50-move rule (halfmove clock, reset on pawn move or capture)
  - Insufficient material draw (K vs K, K + minor vs K)
  - Promotion choice: 4 LEDs on player's back rank let you pick Q/R/B/N (Human-vs-Human only)
  - Pinned-piece detection (legal-move filter prevents exposing own king)
- **Sensor debounce**: 3 consecutive identical reads required before flipping public state. Tunable via `SENSOR_DEBOUNCE_SCANS` in `board_driver.h`. Eliminates the piece-flicker that the original firmware showed when sliding pieces.
- **AP shutdown**: `WiFi.end()` is called when entering Chess Moves or Sensor Test. Saves ~100 mA and removes the orphan `OpenChessBoard` AP from your network list. Bot mode still tears down + reopens internally per [PR #9](https://github.com/Concept-Bytes/Open-Chess/pull/9).
- **On-boot self-tests** (`ChessEngine::runSelfTests()`): 10 deterministic chess engine tests run before WiFi setup, with PASS/FAIL printed to serial and a red-flash failure indicator. Catches engine regressions before they reach a game.
- **Versioned boot banner**: serial monitor now prints `Firmware: v1.0.0-rp2040` and the fork name on every boot.
- **`GameState` struct**: castling rights, en-passant target, halfmove clock, last move tracking — centralised so they can never be forgotten.
- **`ChessEngine::applyMove()`**: single entrypoint for board mutation. Handles regular moves, captures, castling (moves the rook too), en-passant (removes the captured pawn from its actual square), and updates all `GameState` fields. All callers (`chess_moves.cpp`, `chess_bot.cpp`) now go through this.

### Fixed

- **AI mode hang at "Connecting to WiFi…"** ([PR #9](https://github.com/Concept-Bytes/Open-Chess/pull/9)): WiFiNINA on the Nano RP2040 cannot run AP and STA simultaneously. The original firmware called `WiFi.begin()` without shutting down the AP first, causing it to silently never reach `WL_CONNECTED`. Fix: explicit `WiFi.disconnect()` + `WiFi.end()` + `delay(2000)` before `WiFi.begin()`. Closes [issue #5](https://github.com/Concept-Bytes/Open-Chess/issues/5).
- **Stockfish parser broken on Cloudflare-fronted responses** ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)): the API response includes JSON-shaped headers (`Nel:`, `Report-To:`) before the body. The original parser did `indexOf("{")` on the full HTTP response and grabbed the wrong object. Fix: split body from headers on `\r\n\r\n` first.
- **Easy and Medium AI were identical** ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)): `StockfishSettings::medium()` returned `depth=6` despite the serial banner saying `Medium (Depth 10)`. Fixed: `depth=10` to match.
- **Bot move applied without local validation** ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)): if the API returned a malformed or stale move (rare but possible on Cloudflare hiccups), the board state would silently corrupt. Fix: every API response is now run through `ChessEngine::isLegalMove()` before being applied. Rejected moves return the turn to the player.
- **MODE_GAME_3 spam loop** ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)): selecting the placeholder "Coming Soon" mode caused an infinite serial spam (`Returning to game selection in 3 seconds...` every 3 s) while the piece sat on the selector square. Fix: wait for the sensor to clear before re-arming the selection menu.
- **Inconsistent move array sizes** ([PR #10](https://github.com/Concept-Bytes/Open-Chess/pull/10)): `chess_moves.cpp` used `int moves[28][2]` but `chess_bot.cpp` used `int moves[27][2]` — off-by-one buffer overflow risk for queens with 27 candidate moves. Fix: centralised as `#define MAX_MOVES_PER_PIECE 28` in `chess_engine.h`, used everywhere.
- **`arduino_secrets.h` was tracked in git history** with a placeholder SSID (`Kc iphone`) inherited from upstream. Removed from tracking; `.gitignore` already prevents new commits.

### Changed

- All board mutation in `chess_moves.cpp` and `chess_bot.cpp` now goes through `ChessEngine::applyMove()` instead of touching the `board[][]` array directly. Prevents castling-rook-not-moved and en-passant-pawn-not-removed bugs.
- `chess_bot.cpp::executeBotMove()` now also calls `ChessEngine::getGameResult()` after the bot's move to detect end-of-game; previously the AI mode would happily play forever past checkmate.
- `chess_moves.cpp::update()` rewrites the move-detection state machine to enforce turn order (white starts, alternates) and uses `getLegalMoves` (not `getPossibleMoves`) so pinned pieces can't move.
- README replaced with fork-specific content (was the original Concept-Bytes upstream README).
- Self-tests run **before** WiFi setup so engine regressions surface even if WiFi can't initialise.

### Verified

```
Sketch uses 150799 bytes (0%) of program storage space.
Global variables use 44640 bytes (16%) of dynamic memory.
=== Self-tests complete: 10/10 passed ===
```

Tested on Arduino Nano RP2040 Connect with WiFiNINA firmware 3.0.1, Concept-Bytes PCB v1, arduino-cli 1.4.1, arduino:mbed_nano core 4.5.0.

### Known limitations

- **Promotion choice in bot mode**: still auto-promotes to queen. The Stockfish API string includes a 5th promotion char (`e7e8q`) but the existing `parseMove` only takes the first 4. Small follow-up.
- **3-fold repetition**: not implemented (would need ~2 KB RAM for position history).
- **OTA / Web UI / Lichess**: not possible on this hardware (WiFiNINA / RP2040 limitations). For these, see [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess) (ESP32-based).

---

## [Unreleased]

### Planned for v1.1.0-rp2040 (no ETA)

Small, high-impact items, all RP2040-friendly:

- 5-char API move parsing for bot promotion choice (`e7e8q`)
- Difficulty selection at runtime via 4 selector squares (no recompile)
- Brightness control via Arduino EEPROM emulation
- Async LED animations (state-machine refactor)
- 3-fold repetition (if RAM allows)

### Planned for v1.2.0-rp2040 — Web UI

Medium-effort, very high value: removes the recompile-for-WiFi pain point and adds in-browser monitoring.

- LittleFS-served HTML/CSS/JS via the existing AP/HTTP server in `wifi_manager.cpp`
- Configure WiFi without editing `arduino_secrets.h` + recompiling
- Mode selection from browser
- Live board state + FEN export
- Difficulty selection
- Resign / Draw buttons
- Move history (in-RAM)

Will not match [`joojoooo`](https://github.com/joojoooo/OpenChess)'s depth (no themes, no move sounds, no evaluation graphs) — just enough to remove the recompile pain point.

### Speculative for v1.3.0-rp2040 — Lichess (only if there's demand)

- Online Lichess play via NDJSON streaming over HTTPS long-poll (Lichess Bot API)
- Realistic but fragile on WiFiNINA: heap fragmentation over long games, TLS handshake latency on Cloudflare reconnect (~3-5 s gap), token storage
- Honest estimate: 3-4 sessions of work + tuning
- **Most users wanting Lichess on a physical board are better served by [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess) on an ESP32**, so this is low-priority unless someone files a feature request

### Won't fix (architectural limits of the RP2040)

- **OTA firmware updates** — RP2040 has no dual flash partition (A/B), no `Update.h`. The upstream mbed core attempted a `second_stage_ota` patch and reverted it. A custom bootloader is theoretically possible but has weeks of development cost and a real risk of bricking user boards. The `.uf2` drag-and-drop workflow is the moral equivalent (5 seconds of user time).
- **Web Flasher** — no WebUSB driver path for the RP2040. `.uf2` drag-and-drop replaces it.
- **Lichess WebSocket** transport — not used by Lichess Bot API anyway (NDJSON over HTTPS instead).

If you need any of the above, **use [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess) on an ESP32**. It's the right tool for that job.

See [README roadmap](README.md#-roadmap) and [docs/COMPARISON.md](docs/COMPARISON.md) for the latest.

[v1.0.0-rp2040]: https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.0.0-rp2040
[v1.1.0-rp2040]: https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.1.0-rp2040
