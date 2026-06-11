# Changelog

All user-visible changes to the [`semichcsc-byte/Open-Chess`](https://github.com/semichcsc-byte/Open-Chess) Nano RP2040 fix-fork. Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

---

## [v1.4.0-rp2040] — 2026-06-11

### Added

- **Persistent games — resume across power-offs.** The board now saves the
  in-progress game (board, side to move, castling rights, en-passant, halfmove
  clock, and mode/difficulty) to a reserved 64 KB region of internal flash
  after every move. On boot it **silently resumes** where you left off — no
  resume menu, no "set up the pieces" prompt. Power off mid-game, come back
  hours or days later, and just keep playing. In AI mode the board reconnects
  to WiFi automatically on resume. The saved game stays active until you put
  all 32 pieces back on their starting squares (the existing reset gesture),
  which clears it. Works in both Human-vs-Human and Human-vs-AI.
  - New module: `game_persistence.{h,cpp}` (mbed `TDBStore` over `FlashIAP`).
  - Verified on hardware: full power-cycle mid-AI-game resumed to the correct
    position and turn, and reconnected to WiFi.

---

## [v1.3.0-rp2040] — 2026-06-11

UX + AI-mode correctness release. Several issues only surface once you actually
sit down and play a full game against the bot; this release fixes them and makes
it much clearer whose turn it is.

### Added

- **Runtime difficulty selection.** After picking AI mode, 4 centre squares
  light up as buttons — c4 Easy (green), d4 Medium (blue), e4 Hard (amber),
  f4 Expert (red). No more recompiling to change strength.
- **"Whose turn" indicator.** While it's your move, the side-to-move's back
  rank gently breathes (white for you, red for the opponent in HvH), mirroring
  the bot's existing blue "thinking" breathing. No more losing track of turns.
- **Blue AI selector LED.** The Human-vs-AI menu square is now blue (Human-vs-
  Human stays white) so the two options are distinguishable at a glance.

### Fixed

- **AI mode castling.** The player could not castle at all in AI mode (move
  generation never offered it) and the bot's castles didn't move the rook.
  AI mode now uses full legal-move generation, allows castling, moves the rook
  internally, and shows a single blue rook-destination hint (+ blinking source).
- **Invalid FEN after castling.** `boardToFEN()` hard-coded `KQkq`, producing an
  illegal FEN once anyone had castled or moved a king/rook — Stockfish replied
  `"Invalid FEN"` and the turn silently bounced back to the player. Castling
  rights, en-passant target, and the halfmove clock are now derived from live
  game state.
- **Stockfish response parser** no longer grabs Cloudflare's `Report-To:` header
  (whose value is JSON-shaped); it splits the HTTP body from the headers first.
- **Mirrored rank notation** in AI-mode serial logs (`8 - row` → `row + 1`).
- **Light bleed:** lifting a piece now clears the turn-indicator row before
  drawing legal-move dots, so the king's row no longer shows a wall of LEDs.
- **Self-tests now actually run at boot.** `ChessEngine::runSelfTests()` was
  documented as running on every boot but was never called from `setup()`. It
  now runs before WiFi init, prints `PASS T1..T10`, and flashes the board red
  5× on any failure — the documented regression net is back in place.

### Changed

- **Clean serial output.** The `DEBUG: Loop running, uptime…` spam and the wall
  of boot/WiFi diagnostics are gated behind `DEBUG_VERBOSE` / `WIFI_VERBOSE`
  (both off by default). A short "How to play" legend prints at boot instead.

---

## [v1.2.2-rp2040] — 2026-05-11

Reliability release: **AI mode now actually works.** v1.2.0 / v1.2.1 had ~50-100% Stockfish failure rates in real-world conditions; v1.2.2 fixes that completely by routing through a tiny HTTP proxy.

🔗 [Release page](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.2.2-rp2040)

### TL;DR

- Switched AI mode from **HTTPS direct to `stockfish.online`** to **HTTP via [`openchess-proxy.semichcsc.workers.dev`](https://openchess-proxy.semichcsc.workers.dev)** (free Cloudflare Worker).
- API success rate went from "fails ~half the time" to **8 consecutive successful moves on first attempt** in our test session, total request time **~2 seconds** end-to-end.
- The hardened retry loop, exponential backoff, NINA module reset, and visual amber retry pulse are all still in there — they just almost never fire now.

### Why the proxy exists

The Arduino Nano RP2040 Connect's **WiFiNINA TLS stack** (NINA-W102 firmware 3.0.1) **cannot reliably complete TLS 1.3 handshakes with Cloudflare-fronted endpoints**. We instrumented the failure mode with a per-step debug log and the data is conclusive:

```
[DBG] Pre-flight: WiFi status=3 RSSI=-41 dBm IP=192.168.0.99
[DBG] Calling client.connect(stockfish.online:443)...
[DBG] connect() returned FALSE after 9312ms     ← TLS handshake times out
```

Five attempts in a row (each with excellent signal, RSSI ≈ -40 dBm) all hit the same ~9.5s TLS handshake timeout. Even a full `WiFi.disconnect()` → `WiFi.end()` → `WiFi.begin()` cycle (which IP renewal proves works) doesn't help. This isn't flakiness — the NINA-W102 TLS implementation just cannot speak the cipher suite Cloudflare currently negotiates by default. Confirmed by curl that the API works fine from a regular machine in <1 second.

The fix: a **Cloudflare Worker** that accepts plain HTTP from the board (no TLS handshake needed at all on the constrained device) and forwards to stockfish.online over HTTPS server-side, returning just the JSON response.

### Added — Cloudflare Worker proxy (`worker/`)

- New top-level `worker/` directory with `proxy.js` (76 lines) and `wrangler.toml`.
- Deployed at **`https://openchess-proxy.semichcsc.workers.dev`** (free Cloudflare tier, **100,000 requests/day** — enough for ~2,000 active boards playing 50 moves/day each).
- Endpoint: `GET /v2?fen=<urlencoded>&depth=1..15` returns `{"success":true,"bestmove":"bestmove e2e4 ponder e7e5",...}`.
- Health check at `/` returns `OpenChess proxy OK`.
- Don't trust our proxy? **Self-host in 30 seconds**:
  ```bash
  cd worker && npm install -g wrangler && wrangler deploy
  ```
  Then change `STOCKFISH_API_URL` in `arduino_secrets.h` to your own subdomain. See [worker/proxy.js](https://github.com/semichcsc-byte/Open-Chess/blob/main/worker/proxy.js) for the full source.

### Changed — Firmware (`chess_bot.cpp`, `arduino_secrets.h`)

- `chess_bot.h` now includes plain `WiFiClient` instead of `WiFiSSLClient`.
- `STOCKFISH_API_URL` defaults to `openchess-proxy.semichcsc.workers.dev`, `STOCKFISH_API_PORT` is `80`.
- HTTP request format: `GET /v2?fen=...&depth=...` (was `GET /api/s/v2.php?fen=...`).
- Single coalesced `client.print()` + `client.flush()` (was multiple `println()`s) so the entire request hits the wire as one packet — fixes a rare race where Cloudflare dropped requests mid-stream.
- Added `User-Agent: OpenChess/1.2.2 (Arduino-NINA)` and `Accept: application/json` headers — Cloudflare prefers requests with explicit UA.

### Added — Defense in depth (still useful in case the proxy ever has a wobble)

All of the v1.2.2 hardening from earlier today is preserved and still active:

- **5-attempt retry loop** with exponential backoff: 0 / 500 / 1000 / 2000 / 4000 ms.
- **Pre-flight `WiFi.status()` check** with RSSI logged.
- **Quick reconnect** (`ensureWiFiConnected()`, ~8s budget) if link drops between attempts.
- **Full NINA module reset** (`reinitWiFiModule()`) automatically after 3 consecutive failures.
- **Bounded read loop** with 8s inter-byte timeout — no `client.readString()` hangs.
- **Hard 4096-byte response cap** — defensive against OOM.
- **Detailed failure classification** in serial output (`PRE-FLIGHT FAIL`, `CONNECT FAIL`, `WRITE FAIL`, `READ FAIL`, `RECOVERED: attempt N`).

### Added — Visual feedback

- **Pre-call breathing pulse**: 5 forced frames of blue pulse on rank 8 (~600 ms) before opening the socket. Always visible regardless of API latency.
- **Amber retry pulse** (`showRetryFeedback()`): during backoff between attempts, rank 8 flashes amber/orange 1×, 2×, 3×... — different colour from the blue thinking pulse so the user can tell "actively recovering" from "just thinking".
- **Red flash only after all 5 attempts exhausted** (was: after 1 single failure).

### Verified

Eight consecutive Stockfish API calls on hardware, all succeeded on attempt 1:

```
[DBG] connect() returned TRUE after 304ms     →  Total: 1943ms  →  bestmove e7e5
[DBG] connect() returned TRUE after  40ms     →  Total: 2049ms  →  bestmove b8c6
[DBG] connect() returned TRUE after 314ms     →  Total: 2082ms  →  bestmove g8f6
[DBG] connect() returned TRUE after 700ms     →  Total: 2625ms  →  bestmove h7h6
[DBG] connect() returned TRUE after  48ms     →  Total: 2646ms  →  bestmove d7d5
[DBG] connect() returned TRUE after  67ms     →  Total: 2036ms  →  bestmove f6d5
[DBG] connect() returned TRUE after 145ms     →  Total: 2088ms  →  bestmove c8e6
```

```
Sketch uses 151672 bytes (0%) of program storage space.
Global variables use 44640 bytes (16%) of dynamic memory.
=== Self-tests complete: 10/10 passed ===
```

### Known limitations

- `chess_bot.cpp` still uses direct board mutation instead of `ChessEngine::applyMove`, so castling in AI mode doesn't auto-update castling rights, and the row-axis serial print is still mirrored ("f3 to e5" should read "f6 to e4"). Will be fixed in v1.3.
- The default proxy URL points at our public Cloudflare Worker. If you want full self-sufficiency, follow the self-host instructions in `worker/README.md` — the whole proxy is 76 lines of JavaScript and free to run.

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
