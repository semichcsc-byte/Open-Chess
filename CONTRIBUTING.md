# Contributing

Thanks for considering a contribution to the **Nano RP2040 fix-fork** of OpenChess.

## Where to file what

| You found... | File issue here |
|---|---|
| Firmware bug (compile error, runtime crash, wrong move detection, AI hang, etc.) | **This repo** — [open an issue](https://github.com/semichcsc-byte/Open-Chess/issues) |
| Self-test failure on boot (red flash, < 10/10) | **This repo** — include the serial output at 9600 baud |
| Wrong information in [`docs/MANUAL.md`](docs/MANUAL.md) or [`docs/COMPARISON.md`](docs/COMPARISON.md) | **This repo** — PR welcome |
| Build documentation, BOM prices, photos, journal | [`semichcsc-byte/openchess`](https://github.com/semichcsc-byte/openchess/issues) |
| 3D model issue | [MakerWorld page](https://makerworld.com/en/models/1256302-openchess-smart-chess-board?from=search#profileId-1279762) (Concept_Bytes) |
| ESP32 issue (this fork doesn't target ESP32) | [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess/issues) |

## Issue template — what to include

A good firmware bug report has:

- **Hardware:** PCB version (`v1`?), MCU (Nano RP2040 Connect?), magnet size (10×2 mm?)
- **Firmware version:** the line printed on the serial banner — `v1.0.0-rp2040`, etc.
- **WiFi state if AI mode:** SSID, frequency band (2.4 GHz only — WiFiNINA can't do 5 GHz), router model if relevant
- **Serial output at 9600 baud**, from the boot banner to the moment things go wrong
- **What you did**: which mode, which move, which square
- **What you expected vs what happened**
- **Self-test result**: did the 10/10 PASS? Or did it red-flash?

A photo of the board if a sensor or LED is misbehaving helps a lot.

## Code conventions

- C++ for Arduino. Classic Arduino style (no exceptions, `String` is allowed but be aware of heap fragmentation).
- 4-space indentation, `if (cond) {` bracing.
- Public API in `*.h` files; implementation in `*.cpp`.
- Engine code (`chess_engine.*`) is pure C++ with no hardware dependencies — keep it that way so it stays testable from the boot self-tests.
- Hardware code (`board_driver.*`) keeps platform `#ifdef`s minimal.
- Add a self-test in `ChessEngine::runSelfTests()` for any new chess rule. The boot test is our regression net.

## Adding a chess rule / engine change

1. Add the rule to `chess_engine.cpp` / `chess_engine.h`.
2. Add at least one self-test in `runSelfTests()` covering both the positive case (rule works) and the negative case (rule isn't applied where it shouldn't be). Increment the test count in the `Serial.print(... + " passed");` line.
3. Update [`docs/MANUAL.md`](docs/MANUAL.md) if it changes the user experience.
4. Update [`docs/COMPARISON.md`](docs/COMPARISON.md) if it adds or removes a feature relative to the other forks.
5. Bump the version macro in `OpenChess.ino` (`OPENCHESS_FW_VERSION`).
6. Add a [CHANGELOG.md](CHANGELOG.md) entry.

## Adding hardware / driver code

1. Keep platform-specific code behind `#if defined(ARDUINO_NANO_RP2040_CONNECT)` etc., so the codebase compiles for at least the Nano 33 IoT and MKR WiFi 1010 even if untested.
2. **Don't** introduce ESP32-specific dependencies. If you want ESP32, use [`joojoooo/OpenChess`](https://github.com/joojoooo/OpenChess) — that's the right project for it.
3. **Never** commit `arduino_secrets.h`. The `.gitignore` excludes it; double-check before pushing.

## How to submit a PR

1. Fork the repo.
2. Branch off `main`: `git checkout -b feat/your-thing`.
3. Make your changes. Compile locally:

   ```bash
   arduino-cli compile --fqbn arduino:mbed_nano:nanorp2040connect .
   ```

4. Flash to your board if possible and confirm the boot self-tests still pass 10/10.
5. Commit with a clear message; reference the issue number if any.
6. Push to your fork and open a PR against `main`.
7. Describe what you changed, why, what you tested. Include a serial monitor snippet if it's a runtime behaviour change.

I'll review and merge if it's clean. If your PR also makes sense for upstream Concept-Bytes, I'll cherry-pick it onto a separate branch and propose it there too (most likely it sits in their queue forever, but that's better than nothing).

## Building releases

Releases are tagged manually. To produce a `.uf2`:

```bash
arduino-cli compile --fqbn arduino:mbed_nano:nanorp2040connect --output-dir /tmp/build .
cp /tmp/build/OpenChess.ino.uf2 OpenChess-v<VERSION>.uf2
gh release create v<VERSION> --title "v<VERSION>" --notes-file CHANGELOG.md OpenChess-v<VERSION>.uf2
```

(There's no GitHub Actions CI right now — automating this would be a welcome PR.)

## Code of conduct

Be civil. The other forks (Concept-Bytes upstream, joojoooo) are not enemies; they're solving different problems with different trade-offs. Critique the work, never the people.

## License

By contributing, you agree your changes will be licensed under MIT, the same as the rest of the project.
