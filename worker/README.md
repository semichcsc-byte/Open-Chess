# OpenChess Stockfish Proxy

A 76-line Cloudflare Worker that lets the OpenChess board talk to [stockfish.online](https://stockfish.online) over **plain HTTP** instead of HTTPS.

## Why this exists

The Arduino Nano RP2040 Connect's WiFiNINA TLS stack (NINA-W102 firmware 3.0.1) **cannot reliably complete TLS 1.3 handshakes with Cloudflare-fronted endpoints**. Handshakes time out after ~9.5 seconds on a large fraction of attempts even with excellent signal. See [v1.2.2 release notes](https://github.com/semichcsc-byte/Open-Chess/releases/tag/v1.2.2-rp2040) for the full diagnostic story.

This Worker:
- Accepts plain HTTP from the board (no TLS = no problem)
- Forwards the request to `stockfish.online` over HTTPS server-side
- Returns just the JSON body (Cloudflare-side response headers stripped — saves bytes on the constrained device)

## Hosted version (default)

We run a public instance at:

```
https://openchess-proxy.semichcsc.workers.dev
```

Free Cloudflare tier: **100,000 requests/day**, more than enough for a few thousand actively-playing boards. The default firmware points at this URL out of the box, so most users do nothing.

## Self-host (recommended for production / privacy)

Don't trust our proxy? Want full self-sufficiency? Deploy your own copy in 30 seconds:

```bash
cd worker
npm install
npx wrangler login          # opens browser, log in to your free Cloudflare account
npx wrangler deploy
```

Output will tell you your Worker URL, e.g. `https://openchess-proxy.<your-subdomain>.workers.dev`.

Then in `arduino_secrets.h`:

```c
#define STOCKFISH_API_URL "openchess-proxy.<your-subdomain>.workers.dev"
```

Recompile, flash, done.

## API

### `GET /`
Health check. Returns `OpenChess proxy OK`.

### `GET /v2?fen=<urlencoded-fen>&depth=<1..15>`
Forwards to `stockfish.online/api/s/v2.php`. Returns the JSON body unchanged:

```json
{
  "success": true,
  "evaluation": -1.23,
  "mate": null,
  "bestmove": "bestmove e2e4 ponder e7e5",
  "continuation": "e2e4 e7e5 g1f3 b8c6 ..."
}
```

## License

Same as the firmware: MIT. See [../LICENSE](../LICENSE).
