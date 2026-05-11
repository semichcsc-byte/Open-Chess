# Changelog

All user-visible changes to the [`semichcsc-byte/Open-Chess`](https://github.com/semichcsc-byte/Open-Chess) Nano RP2040 fix-fork. Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

---

## [v1.2.1-rp2040] — 2026-05-11

Patch release: castling is now visually obvious in Human-vs-Human mode.

🔗 [Release page](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.2.1-rp2040)

### Added

- **Castling visual hint** in `chess_moves` (Human-vs-Human mode): when you complete a king-side or queen-side castle by moving the king two squares, the board immediately:
  1. Flashes the rook's source square in **blinking blue** 4 times (so you know which rook to lift).
  2. Lights the rook's destination square in **solid blue** (so you know where to put it).
  3. Holds both lit for an extra 2 seconds so a slow user catches the hint.

  Previously the firmware silently expected you to know the FIDE rules and just "move the rook too" — which is the kind of muscle memory regular tournament players have but most casual players don't. Now the board explicitly tells you what's needed.

  Serial log also prints `Castling: move the rook from h1 to f1` (or appropriate squares) so you can confirm.

### Known limitation

AI mode (`chess_bot.cpp`) does not yet use the same engine `applyMove` path, so the castling visual hint is **Human-vs-Human only** for now. Will be ported to AI mode in v1.3 when `chess_bot.cpp` is refactored to use `ChessEngine::applyMove` (which will also fix the row-axis serial print mirror in one go).

### Verified

```
Sketch uses 148801 bytes (0%) of program storage space.
Global variables use 44640 bytes (16%) of dynamic memory.
=== Self-tests complete: 10/10 passed ===
```

---

## [v1.2.0-rp2040] — 2026-05-11 — **first stable release** ⭐

🔗 **[Download (drag-and-drop `.uf2`)](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.2.0-rp2040)**

This is the **canonical release** for users with the [Concept-Bytes OpenChess PCB v1](https://concept-bytes.com/products/openchess-pcb) and the Arduino Nano RP2040 Connect (the Kickstarter / official kit). It supersedes the four iteration releases on this same day (v1.0.0-rp2040 through v1.1.2-rp2040) — they remain in the history below as a record but you should grab v1.2.0 instead.

### What this firmware does (vs the abandoned upstream)

#### Working AI mode
- 🌐 **Stockfish AI via WiFi**: connects to your home network, plays at Easy / Medium / Hard / Expert depth via the free [stockfish.online](https://stockfish.online) public API.
- 🛡️ **Local move validation**: every move the API returns is run through the on-board chess engine before being applied. Malformed responses, partial reads, side-of-turn confusion, etc. all return the turn cleanly to the player.
- 🟦 **Bot-thinking pulse**: slow blue breathing on the bot's back rank while Stockfish is computing, so the board never looks frozen.
- 🟥 **API failure feedback**: if the bot can't reach Stockfish (timeout, network blip, TLS failure), rank 8 flashes red 3 times and the turn returns to the player so the move can be retried.
- 🔌 **Resilient WiFi setup**: clean AP→STA tear-down before `WiFi.begin()` (the upstream silently hangs on this; see [PR #9](https://github.com/Concept-Bytes/Open-Chess/pull/9)).

#### Full chess rules
- ✅ Check, checkmate, stalemate detection with on-board animations
- ✅ Castling (kingside + queenside, FIDE-legal — king-not-in-check, king-doesn't-pass-through-attacked, rights tracked properly)
- ✅ En passant with pink LED hint and correct captured-pawn removal
- ✅ Pinned-piece detection (legal-move filter prevents exposing own king)
- ✅ 50-move rule + insufficient material draw (K-vs-K, K + minor vs K)
- ✅ Promotion choice: 4 LEDs on the player's back rank let you pick Q / R / B / N (Human-vs-Human mode)

#### UX
- 💡 **Setup hint at boot**: 16 LEDs glow soft white on the white side, 16 glow red on the black side. As you place each piece, the LED for that square goes dark.
- 🌈 **Convergent rainbow explosion** when all 32 pieces are in place (4 collapsing rainbow rings → 4 diagonal beams from corners → white shockwave outward → triple pulse on the 2 selectors). ~3.5 s of spectacle.
- 🎯 **Simplified 2-option menu**: D5 lights for Human-vs-Human, E4 for AI mode.
- 📌 **Sticky menu**: lifting a piece to use as a selector no longer regresses to the setup hint.
- 🔄 **Reset gesture**: while playing, putting all 32 pieces back to starting squares for ≥1.5 s drops back to the menu (re-runs the explosion). Lets you finish a game and start a different mode without power-cycling.
- 🆔 **Versioned boot banner** prints firmware version + fork name on serial.

#### Robustness
- ✅ **10 self-tests** at every boot: pseudo-legal moves, legal-move filtering (own-check), Fool's Mate detection, pinned-piece, castling rights, castling-in-check forbidden, en-passant, K-vs-K draw, `applyMove` correctness for castling. Failures flash red 5×.
- ✅ **Sensor debounce** (3 consecutive identical reads required to flip state) — eliminates the piece-flicker upstream had on slide.
- ✅ **AP shutdown** when not needed (saves ~100 mA, removes the orphan `OpenChessBoard` WiFi network).
- ✅ **MODE_GAME_3 placeholder fix**: no longer spams serial at ~3 lines/sec when a piece sits on the placeholder square.
- ✅ **Driver-layer column-mirror fix**: the Concept-Bytes PCB wires file `a` to internal column 7. The original firmware never accounted for this, so all chess notation in serial logs / FEN strings was mirrored across the a-h axis. Fixed in `BoardDriver` (1 line in `readSensors`, 1 line in `getPixelIndex`); all 36 callers untouched.

### Bug fixes vs upstream Concept-Bytes (each was a real PR upstream)

| # | Bug | PR | Fix |
|---|---|---|---|
| 1 | AI mode hangs at "Connecting to WiFi…" | [#9](https://github.com/Concept-Bytes/Open-Chess/pull/9) | AP→STA tear-down before `WiFi.begin()` |
| 2 | "API request was not successful" on success | [#10](https://github.com/Concept-Bytes/Open-Chess/pull/10) | Parser splits HTTP body from headers |
| 3 | Easy and Medium AI are identical depth | [#10](https://github.com/Concept-Bytes/Open-Chess/pull/10) | `medium()` now sends depth=10 |
| 4 | Bot move applied without local validation | [#10](https://github.com/Concept-Bytes/Open-Chess/pull/10) | Engine validates before mutating board |
| 5 | MODE_GAME_3 placeholder spams serial | [#10](https://github.com/Concept-Bytes/Open-Chess/pull/10) | Wait for piece-lift before re-arming |
| 6 | `moves[27]` vs `moves[28]` inconsistency | [#10](https://github.com/Concept-Bytes/Open-Chess/pull/10) | Centralised `MAX_MOVES_PER_PIECE` |
| 7 | No chess rules (check, mate, castling, EP, draws) | [#11](https://github.com/Concept-Bytes/Open-Chess/pull/11) | Full FIDE engine |
| 8 | Bot freezes silently on API timeout | this fork only | All failure paths reset `isWhiteTurn = true` + red flash |
| 9 | Column-mirror coordinate bug | this fork only (PCB-specific calibration finding) | Mirror `col` in `BoardDriver` |

### Verified

```
Sketch uses ~150 KB (0% of 16 MB program flash).
Global variables use ~44 KB (16% of 270 KB RAM).
=== Self-tests complete: 10/10 passed ===
```

End-to-end on Arduino Nano RP2040 Connect with WiFiNINA firmware 3.0.1:
- 10 moves of an Italian Game (with capture, invalid-move recovery, breathing pulse during bot-thinking) all worked
- Reset gesture validated mid-game
- Network kill mid-move triggers red-flash and returns turn to player

### Hardware tested

- **Concept-Bytes PCB v1** (the one shipped to Kickstarter backers)
- **Arduino Nano RP2040 Connect**
- **WiFiNINA firmware 3.0.1**
- **arduino-cli 1.4.1**, `arduino:mbed_nano` core 4.5.0
- **Adafruit NeoPixel** library v1.14.0 (pinned)

### Known limitations (deferred to v1.3+)

- **Serial debug print has a row-axis mirror**: a move from `e2 to e4` is logged as `from e7 to e5`. Internally the firmware tracks the position correctly and Stockfish receives the right FEN — only the human-readable serial print has a row-axis mirror, the same kind of bug that was fixed for columns. Easy fix in `chess_bot.cpp::processPlayerMove` and `chess_moves.cpp` print helpers.
- **AI mode promotion**: pawn always auto-promotes to queen (5-char API parse for `e7e8q` not implemented yet).
- **Difficulty selection** still requires a recompile; no runtime selector.
- **3-fold repetition** not implemented (would need ~2 KB RAM for position history).

For a planned-vs-won't-fix breakdown including Web UI / OTA / Lichess, see [`docs/COMPARISON.md`](docs/COMPARISON.md).

---

## Detailed iteration history (intra-day, 2026-05-11)

These four releases were the iteration steps that built up to v1.2.0. They're kept here as a historical record and remain downloadable from the [releases page](https://github.com/semichcsc-byte/Open-Chess/releases) but **users should install v1.2.0** above.

| Tag | Theme |
|---|---|
| [v1.0.0-rp2040](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.0.0-rp2040) | First tagged build: PRs #9, #10, #11 combined |
| [v1.1.0-rp2040](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.1.0-rp2040) | Column-mirror coordinate fix + UX overhaul (setup hint, rainbow explosion, 2-option menu, sticky menu, reset gesture) |
| [v1.1.1-rp2040](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.1.1-rp2040) | Bot-thinking breathing pulse on rank 8 (and the SSL-handshake corruption fix that the first attempt at this animation introduced) |
| [v1.1.2-rp2040](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.1.2-rp2040) | Bot survives Stockfish API timeout (was freezing silently) |
| **v1.2.0-rp2040** | **Consolidated stable release** ⭐ |

[v1.2.0-rp2040]: https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.2.0-rp2040
[v1.2.1-rp2040]: https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.2.1-rp2040
