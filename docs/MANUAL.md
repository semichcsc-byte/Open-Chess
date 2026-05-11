# OpenChess User Manual (v1.0.0-rp2040 firmware)

This is the user manual for the **patched firmware** in this repo ([`semichcsc-byte/Open-Chess`](https://github.com/semichcsc-byte/Open-Chess)) running on a Concept-Bytes PCB + Arduino Nano RP2040 Connect.

If you're reading this with the original Concept-Bytes firmware, several features described here won't work. [Flash the patched firmware](../README.md#-quick-start-60-seconds) first.

## Table of contents

- [First boot](#first-boot)
- [Mode selection](#mode-selection)
- [Game mode 1 — Human vs Human](#game-mode-1--human-vs-human)
  - [Castling](#castling)
  - [En passant](#en-passant)
  - [Promotion](#promotion-choice)
  - [Check, checkmate, draws](#check-checkmate-draws)
- [Game mode 2 — Human vs AI (Stockfish)](#game-mode-2--human-vs-ai-stockfish)
- [Game mode 3 — Coming Soon](#game-mode-3--coming-soon)
- [Game mode 4 — Sensor Test](#game-mode-4--sensor-test)
- [WiFi & AI mode setup](#wifi--ai-mode-setup)
- [Self-tests](#self-tests)
- [Troubleshooting](#troubleshooting)
- [Resetting the board](#resetting-the-board)
- [Coordinate system](#coordinate-system)

---

## First boot

When you power on the board (USB-C to the Arduino, or external 5 V to J1 if you've populated it):

1. **LEDs all off** for a moment while the firmware boots.
2. **Self-tests run** (see [Self-tests](#self-tests)) — pure software, no LED activity unless something fails.
3. **WiFi AP `OpenChessBoard` starts** (password `chess123`). You can ignore this; the AP is only useful for a future web UI.
4. **4 white LEDs light up in the centre of the board** — this is the [mode selection](#mode-selection) menu.

If the board flashes red 5 times immediately after boot, **the self-tests failed** — your firmware is broken. Re-flash the latest release.

---

## Mode selection

After boot, four white LEDs light up in the centre of the board. Each LED is a button: **place any chess piece on top of one to select that mode**.

```
   a   b   c   d   e   f   g   h
 8 .   .   .   .   .   .   .   .  8
 7 .   .   .   .   .   .   .   .  7
 6 .   .   .   .   .   .   .   .  6
 5 .   .   .   ⚪  ⚪  .   .   .  5     ⚪ = white selector LED
 4 .   .   .   ⚪  ⚪  .   .   .  4
 3 .   .   .   .   .   .   .   .  3
 2 .   .   .   .   .   .   .   .  2
 1 .   .   .   .   .   .   .   .  1
```

| LED | Internal coordinates | Mode |
|-----|----------------------|------|
| **D5** | row 3, col 3 | 1 — Human vs Human |
| **E5** | row 3, col 4 | 2 — Human vs AI (Stockfish) |
| **D4** | row 4, col 3 | 3 — Coming Soon (placeholder) |
| **E4** | row 4, col 4 | 4 — Sensor Test |

> **Tip:** the four selector LEDs form a 2×2 square at the very centre of the board. With the white pieces (rank 1) closer to you, the LEDs are arranged as:
>
> ```
>  Human vs Human (D5)   Human vs AI (E5)         ← back row of selector
>  Coming Soon    (D4)   Sensor Test (E4)         ← front row of selector
> ```

After selection:

- The 4 selector LEDs go off
- A green flash confirms the choice
- The mode-specific intro animation plays
- For Human-vs-Human and Human-vs-AI: the board waits for you to set up all 32 pieces in starting position before allowing moves

---

## Game mode 1 — Human vs Human

### Setting up

1. Select Human vs Human (place a piece on **D5**).
2. The 32 starting squares slowly light up in dim white as you place each piece. **You must place all 32 pieces** before the game starts.
3. When the board is fully set up, a **fireworks animation** plays. Game on.

### Making a move

1. **Lift a piece** of the colour whose turn it is (white starts).
   - The square you lifted from glows **dim white**.
   - Every legal destination glows **soft white** (quiet move) or **red** (capture).
   - For en-passant captures, the destination glows **pink**.
   - If you lift a piece that has **no legal moves** (e.g. a pinned piece), the square blinks twice and you must replace it.
2. **Place the piece on a legal destination**, OR **return it to its original square** to cancel.
3. The destination flashes **green** twice to confirm.
4. Turn switches to the other colour.

> **The firmware enforces all chess rules.** You cannot make an illegal move because illegal squares simply aren't lit. This includes:
> - Pieces pinned against your own king
> - Moves that leave your king in check
> - Castling through attacked squares
> - Castling while in check
> - Castling after the king or rook has moved (rights tracked in `GameState`)

### Castling

Both kingside (O-O) and queenside (O-O-O) castling work:

1. Lift your king. If castling is legal, you'll see white LEDs **two squares away** on the king's flank (g1/g8 for kingside, c1/c8 for queenside) in addition to the normal one-square king moves.
2. Place the king on the castling target square.
3. The board lights up the rook's source and destination — **move the rook to complete the castle** (h1→f1 or a1→d1, mirror for black).
4. Done.

The board prevents any of the FIDE-illegal castles automatically:

- King has moved
- Rook on that side has moved (or has been captured on its starting square)
- King is in check
- King's path is attacked
- Squares between king and rook are not empty

### En passant

If your opponent just made a 2-square pawn advance and your pawn is adjacent:

1. Lift your pawn.
2. The diagonal destination behind their pawn glows **pink** (instead of white) — this is the en-passant target.
3. Place your pawn there.
4. The captured pawn's square is shown briefly in red. **Remove the captured pawn** from the board to complete the move.

The window is exactly one move — if you don't take the en-passant immediately, the right is lost.

### Promotion choice

When your pawn reaches the back rank:

1. The destination square lights up gold.
2. **Four selector LEDs light up on your own back rank** (rank 1 for white, rank 8 for black) on files a, b, c, d:

   | File | Colour | Promotes to |
   |------|--------|-------------|
   | **a** | Gold | Queen |
   | **b** | Red | Rook |
   | **c** | Blue | Bishop |
   | **d** | White | Knight |

3. **Place any chess piece on one of the four selector squares.**
4. Lift the selector piece off when prompted. The pawn is now treated as the chosen piece for the rest of the game (you can swap the physical pawn for a queen, but the firmware doesn't require it — it tracks the promoted type internally).

### Check, checkmate, draws

- **Check:** the king square blinks red 3 times. The next move must remove the check (block, capture, or move king).
- **Checkmate:** game over animation plays. White wins → entire board glows white. Black wins → red pulse 3×.
- **Stalemate / 50-move rule / insufficient material:** fireworks animation. The board prints `DRAW - <reason>` on the serial monitor.

To start a new game after game over: lift all the pieces and re-select a mode (the board auto-resets state).

> **Not yet implemented:** 3-fold repetition (would require position hashing + history; possible but uses more RAM).

---

## Game mode 2 — Human vs AI (Stockfish)

### Prerequisites

- WiFi credentials configured in `arduino_secrets.h` and the firmware re-flashed (see [WiFi & AI mode setup](#wifi--ai-mode-setup)).
- Internet access to `stockfish.online` (HTTPS).

### Starting

1. Place a piece on **E5** (the Human vs AI selector).
2. The board prints the bot difficulty (Medium = Stockfish depth 10 by default).
3. The board tears down its own AP, then connects to your home WiFi.
   - On success: 3 brief green flashes across the entire board.
   - On failure: 5 red flashes, AI mode is unavailable until next reboot. See [Troubleshooting](#troubleshooting).
4. Set up the 32 pieces in starting position. Fireworks play. White (you) starts.

### Playing

- You play **white**. The bot plays **black**.
- Make your move exactly as in [Human vs Human](#making-a-move).
- The board sends the position (FEN) to the Stockfish API and waits for the bot's response (typically <2 s for Medium, longer for Hard/Expert).
- The bot's intended move is shown on the board:
  - **Source square** blinks white.
  - **Destination square** glows white.
- **You make the move physically:** lift the bot's piece from the source, place it on the destination.
- The board confirms with green flashes. Your turn again.

### What if the bot's move is illegal?

The patched firmware **validates every API response locally** with the chess engine. If Stockfish ever returns a move that isn't legal in the current position (corrupt response, partial JSON, side-of-turn confusion), the board:

- Prints `Bot move <xxxx> rejected by local engine` to serial
- Returns the turn to you (you don't lose your move)
- Stays in the current position

This protects against silent state corruption.

### Difficulty

By default the bot plays at Medium (Stockfish depth 10, ~25 s timeout). Easy/Hard/Expert are wired in `chess_bot.cpp::ChessBot()` constructor and `stockfish_settings.h`:

| Difficulty | Stockfish depth | Timeout |
|---|---|---|
| Easy   | 6  | 15 s |
| Medium | 10 | 25 s |
| Hard   | 14 | 45 s |
| Expert | 16 | 60 s |

To change difficulty you currently need to edit `chess_bot.cpp` and re-flash. A future revision could pick the difficulty from the 4 selector squares post-mode-select.

### What's not yet implemented in AI mode

- **Promotion choice for the player.** When your pawn reaches the back rank in AI mode, it auto-promotes to Queen. (The Stockfish API string includes a 5th promotion char like `e7e8q` but the parser only takes the first 4 chars; bolting on the 5th is a small follow-up.)
- **Showing the bot's promotion choice on the LED.** Same reason.

---

## Game mode 3 — Coming Soon

This is a placeholder for a future game mode (likely a chess puzzle / tactics trainer). Selecting it just prints a message and returns to the menu when you lift the piece.

> **Note:** in the original Concept-Bytes firmware, this mode caused an infinite loop spamming serial and freezing the menu while the piece was on the square. The patched firmware fixes that — it waits for you to lift the piece before re-arming the menu.

---

## Game mode 4 — Sensor Test

Place a piece on **E4**. The board enters a continuous loop where every square that has a magnet on it lights up blue.

Use this to:

- Test that all 64 hall sensors detect their magnet
- Diagnose dead zones (squares that don't light up even with a magnet)
- Verify magnet polarity (A3144 sensors are unipolar — only the **south pole** triggers them; if a piece doesn't register, flip it over)

To exit: power-cycle the board (no menu return from this mode).

---

## WiFi & AI mode setup

The pre-built `.uf2` does **not** include WiFi credentials — we can't ship binaries with someone else's password baked in. To use AI mode you need to compile the firmware once with your home WiFi configured. Once that's done, the credentials are stored in flash and survive power cycles. **You only need to do this once** unless you change WiFi network or move house.

### Prerequisites

- A 2.4 GHz WiFi network with internet access. **WiFiNINA cannot connect to 5 GHz networks** — most modern routers broadcast both bands, but if yours is 5 GHz only, you need to enable 2.4 GHz in the router admin.
- The WiFi password (case sensitive). Special characters work but **must not contain** unescaped backslashes (`\`) or unbalanced quotes (`"`). Most ordinary passwords are fine.
- A USB-C cable to flash.
- The Arduino CLI installed (`brew install arduino-cli` on macOS, or [arduino-cli releases](https://arduino.github.io/arduino-cli/latest/installation/) elsewhere).

### Quick setup (5 minutes, one time)

#### 1. Clone the firmware fork

```sh
git clone https://github.com/semichcsc-byte/Open-Chess.git
cd Open-Chess
git checkout v1.1.0-rp2040    # or whatever the latest tag is
```

#### 2. Create your secrets file

```sh
cp arduino_secrets_template.h arduino_secrets.h
```

#### 3. Edit `arduino_secrets.h`

Open the file in any text editor and fill in the WiFi network you want the bot to connect to:

```c
#ifndef ARDUINO_SECRETS_H
#define ARDUINO_SECRETS_H

// WiFi Network Credentials
#define SECRET_SSID "YourWiFiName"          // case sensitive
#define SECRET_PASS "YourWiFiPassword"      // case sensitive

// Stockfish API — these defaults work, leave them alone unless
// stockfish.online ever moves to a new endpoint.
#define STOCKFISH_API_URL  "stockfish.online"
#define STOCKFISH_API_PATH "/api/s/v2.php"
#define STOCKFISH_API_PORT 443  // HTTPS

#endif
```

> ⚠️ **Common mistakes that will silently fail with `WL_CONNECT_FAILED`:**
> - **Wrong band**: WiFiNINA can't see 5 GHz networks. If your phone connects to `MyWiFi` but the board doesn't, check your router has a 2.4 GHz SSID enabled (often the same name with `_2.4G` suffix on dual-band setups).
> - **Hidden SSID**: WiFiNINA can connect to hidden SSIDs but it's flaky. If possible, make the network broadcast its name.
> - **Captive portal / open network**: Hotel and airport WiFi that requires a browser login won't work — there's no browser on the board.
> - **WPA Enterprise (802.1X)**: not supported. Home WPA2 / WPA3 PSK is fine.
> - **Phone hotspot**: works, but your phone needs to stay on the same network the whole time you play.

#### 4. Install dependencies (one-time)

```sh
arduino-cli core install arduino:mbed_nano
arduino-cli lib install "Adafruit NeoPixel"@1.14.0
arduino-cli lib install WiFiNINA
```

#### 5. Find your board's USB port

Plug in the Arduino, then:

```sh
arduino-cli board list
```

You'll see something like:

```
Port                            Protocol Type              Board Name
/dev/cu.usbmodem201301          serial   Serial Port (USB) Arduino Nano RP2040 Connect
```

The path you want is `/dev/cu.usbmodemXXXXXXX` (macOS), `/dev/ttyACM0` (Linux), or `COMx` (Windows).

#### 6. Compile and upload

```sh
arduino-cli compile --fqbn arduino:mbed_nano:nanorp2040connect .
arduino-cli upload --fqbn arduino:mbed_nano:nanorp2040connect -p /dev/cu.usbmodem201301 .
```

(Replace `usbmodem201301` with whatever your `board list` showed.)

After the upload finishes the board reboots automatically with the new firmware. Open the Serial Monitor at 9600 baud and you should see, after a few seconds:

```
=== Starting Chess Bot Mode ===
Tearing down AP mode before connecting as station...
Attempting to connect to SSID: YourWiFiName
Connection attempt 1/10 - Status: 3
Connected to WiFi!
IP address: 192.168.x.y
WiFi connected! Bot mode ready.
```

`Status: 3` = `WL_CONNECTED` ✅. From this point on the AI mode works.

### What if it fails?

The connection log will end with `Failed to connect to WiFi` and the board flashes red 5 times. Match the symptom against this table:

| Serial output | Most likely cause | Fix |
|---|---|---|
| `Connection attempt N/10 - Status: 4` repeating, never gets past 4 | Wrong password (`WL_CONNECT_FAILED`) | Re-check `SECRET_PASS` for typos, case, missing characters at the end. **Special chars `\` and `"` need escaping** (e.g. `"P\\\\ass\\"word"` for `P\ass"word`) |
| `Status: 6` repeating | SSID not found (`WL_DISCONNECTED`) | The board can't see your network. Likely on 5 GHz only, or out of range. Move the Arduino closer to the router; turn on 2.4 GHz |
| `Status: 1` | `WL_NO_SSID_AVAIL` | Same as above |
| Connects then loses connection mid-game | Weak signal, or router does aggressive client kicking | Move closer; on TP-Link / ASUS routers, disable "Smart Connect" / band-steering; whitelist the board's MAC address |
| `WiFi module not found!` | The Arduino can't talk to the WiFiNINA chip | Reset the board (white button), re-flash. If it persists, the U-blox NINA-W102 module on the Arduino has likely come loose — try Arduino's [WiFiNINA firmware updater](https://docs.arduino.cc/tutorials/communication/wifi-nina-examples) |
| Connects but `Failed to connect to Stockfish API` | Internet blocked or DNS issue | From your laptop on the same WiFi, run `curl https://stockfish.online/api/s/v2.php?fen=rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR%20w%20KQkq%20-%200%201&depth=6` — if that fails too, your network is blocking the API |

### Changing WiFi later

If you move house or the router password changes, just repeat steps 3 + 6. No need to re-clone or re-install dependencies.

### Connecting to a phone hotspot

Useful when you want to play AI mode somewhere without home WiFi.

1. On your phone, enable Personal Hotspot.
2. Note the SSID + password (often shown on the hotspot screen).
3. Edit `arduino_secrets.h` with those credentials and re-flash.
4. **Keep the phone awake and on the same network** during play. Some phones throttle the hotspot when the screen is off — disable battery optimisation for the hotspot if so.

### Security note

The compiled `.uf2` / `.bin` artifact contains your WiFi password as plaintext (the firmware can't connect otherwise). **Do not share these binaries**. The release `.uf2` on GitHub is built without credentials and is safe to share — it's the one you should hand to friends.

The `.gitignore` in this repo excludes `arduino_secrets.h` so you can commit other firmware changes without leaking your password.

### How the bot reaches Stockfish

```
Board (Nano RP2040, station mode)
   │
   │ HTTPS GET /api/s/v2.php?fen=…&depth=10
   ▼
Your home router
   │
   ▼
stockfish.online (Cloudflare-fronted)
   │ Returns JSON: {"success":true, "bestmove":"bestmove e2e4 ponder d7d5", …}
   ▼
Board parses, validates, applies move
```

There's no server you need to host — `stockfish.online` is a free public API. Latency is typically < 2 s for depth 10.

---

## Self-tests

Every boot, before WiFi setup, the firmware runs **10 deterministic chess engine tests** and prints results to serial at 9600 baud:

```
=== ChessEngine self-tests ===
PASS T1: e2 pawn has 2 legal moves
PASS T2: b1 knight has 2 legal moves
PASS T3: no check at start
PASS T4: Fool's Mate detected
PASS T5: pinned rook stayed on file
PASS T6: both castlings legal
PASS T7: no castling in check (or fail above)
PASS T8: en-passant offered
PASS T9: K vs K is draw
PASS T10: kingside castle layout correct
=== Self-tests complete: 10/10 passed ===
```

If any test fails:

- The board flashes **red 5 times across all 64 LEDs** (impossible to miss)
- A `WARNING: N engine self-tests FAILED` line is printed
- The board still continues to boot, but you should re-flash a clean release because the engine is broken

The tests cover: pseudo-legal move generation, legal-move filtering (own-check), Fool's Mate detection, pinned-piece handling, castling rights, castling-in-check forbidden, en-passant, insufficient material, and `applyMove` correctness for castling.

These caught a **real bug during development** (a wrong test fixture) and would catch any regression in the engine before it reaches a game.

---

## Troubleshooting

### "AI mode does nothing when I select it"

Check the serial monitor (9600 baud).

| Symptom in serial | Cause | Fix |
|---|---|---|
| No `=== Starting Chess Bot Mode ===` line | You placed the piece on the wrong selector. | E5 = AI (not D5/D4/E4). |
| Stops at `Connecting to WiFi...` | You're running the **unpatched** Concept-Bytes firmware (AP+STA conflict). | Flash v1.0.0-rp2040. |
| `Connection attempt 10/10 - Status: ...` then red flash | Wrong WiFi credentials, or 5 GHz network. WiFiNINA only supports 2.4 GHz. | Edit `arduino_secrets.h`, switch to a 2.4 GHz SSID. |
| `Failed to connect to Stockfish API` | Internet blocked, DNS issue, or `stockfish.online` is down (rare). | Try `curl https://stockfish.online/api/s/v2.php?fen=…&depth=6` from your laptop. |
| `Bot move XXXX rejected by local engine` | Stockfish returned an illegal move (rare; Cloudflare hiccup). | Just play your turn again — the board returned the turn to you. |

### Sensors flicker / wrong piece detected

- The patched firmware adds **debounce (3 consecutive scans)**. If you still see flicker:
  - Slow down piece movement
  - Verify the piece's magnet hasn't fallen off
  - Try the Sensor Test mode to find the bad squares
- If the **same square** consistently fails to detect:
  - Magnet polarity wrong on that piece (flip it)
  - Hall sensor (A3144) damaged on that square — visible as no detection in Sensor Test
  - PCB trace or solder joint issue

### LEDs not lighting up

- The Arduino Nano RP2040 powers the LEDs from its own 3.3 V supply via a level shifter. If you see no LEDs at all:
  - Check the LED data line on D17 (NeoPixel `DIN`)
  - Re-seat the Arduino in its socket
  - Try the Sensor Test mode to force LEDs on

### Board freezes mid-game

If the firmware freezes (no serial output for 30+ seconds):

- Power-cycle (unplug USB, plug back in)
- Re-flash the latest release in case of corruption
- Open an issue with the serial output in [the firmware repo](https://github.com/semichcsc-byte/Open-Chess/issues)

### "I picked the wrong mode by mistake"

Some modes (Sensor Test, AI mode after WiFi connect) don't have a back-to-menu option. **Power-cycle the board** to return to mode selection.

For mode 3 (Coming Soon), just lift the piece — the patched firmware returns to the menu automatically.

---

## Resetting the board

| What you want | How |
|---|---|
| Restart current game | Lift all pieces, set up starting position again. The state auto-resets after game over. |
| Force back to mode selection | Power-cycle (unplug + replug USB). |
| Clear all state and re-flash firmware | Double-tap reset button → drag fresh `.uf2` to `RPI-RP2` drive. |
| Reset to factory firmware | Flash the original Concept-Bytes binary (not recommended — known bugs). |

---

## Coordinate system

The firmware uses an **internal `(row, col)` coordinate system** that maps to standard chess notation as follows:

| Internal | Chess notation |
|---|---|
| `row 0` | rank 1 (white back rank: `RNBQKBNR`) |
| `row 1` | rank 2 (white pawns) |
| `row 2-5` | empty middle ranks |
| `row 6` | rank 7 (black pawns) |
| `row 7` | rank 8 (black back rank) |
| `col 0` | file `a` |
| `col 7` | file `h` |

So the white king's home square `e1` is internally `(0, 4)`, and the black queen `d8` is `(7, 3)`.

The serial output uses standard algebraic notation (`e2`, `d4`, `Qh4#`, …) so you don't usually need to think in `(row, col)`.

---

## Need help?

- 🐛 **Bug reports / feature requests**: [open an issue](https://github.com/semichcsc-byte/Open-Chess/issues) here in this firmware repo
- 💡 **Build questions** (BOM, photos, hardware assembly): see the [build documentation repo](https://github.com/semichcsc-byte/openchess) (BOM, BUILD_LOG, photos)
- 🌐 **Wiring & schematics**: the [`joojoooo` build guide](https://joojoooo.github.io/OpenChess/) is the best public reference (covers both RP2040 and ESP32 layouts)
- � **Hardware & STLs**: PCB from [concept-bytes.com](https://concept-bytes.com/products/openchess-pcb) (originally a [Kickstarter campaign](https://www.kickstarter.com/projects/conceptbytes/open-chess-a-3d-printable-smart-chess-board?ref=cc9981)); 3D files at [MakerWorld](https://makerworld.com/en/models/1256302-openchess-smart-chess-board?from=search#profileId-1279762) (Concept_Bytes, CC BY 4.0)
- �📺 **Original videos**: [Concept Bytes on YouTube](https://www.youtube.com/@conceptbytes)
