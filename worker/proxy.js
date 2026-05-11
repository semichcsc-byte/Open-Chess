/**
 * OpenChess Stockfish HTTP -> HTTPS proxy
 *
 * The Arduino Nano RP2040 Connect's WiFiNINA TLS stack (NINA-W102 firmware
 * 3.0.1) cannot reliably complete TLS 1.3 handshakes with Cloudflare-fronted
 * endpoints like stockfish.online -- the handshake times out after ~9.5s on
 * a large fraction of attempts even with excellent signal.
 *
 * This Worker accepts plain HTTP from the board and forwards to
 * stockfish.online over HTTPS, then returns just the JSON body. Plain HTTP
 * eliminates the TLS handshake entirely on the constrained device.
 *
 * Deployed at:   https://openchess-proxy.semichcsc.workers.dev/
 * Free tier:     100,000 requests/day (more than enough)
 *
 * URL format:
 *   GET /v2?fen=<urlencoded>&depth=<1-15>
 *
 * Response: same JSON body as stockfish.online/api/s/v2.php, e.g.
 *   {"success":true,"evaluation":-1.23,"mate":null,
 *    "bestmove":"bestmove e2e4 ponder e7e5","continuation":"..."}
 *
 * Self-host:
 *   $ npm install -g wrangler
 *   $ wrangler deploy            # uses your own Cloudflare account
 *   Then point chess_bot.cpp's PROXY_HOST at your worker's URL.
 */

const UPSTREAM = 'https://stockfish.online/api/s/v2.php';

export default {
  async fetch(request) {
    const url = new URL(request.url);

    // Health check
    if (url.pathname === '/' || url.pathname === '/health') {
      return new Response('OpenChess proxy OK\n', {
        status: 200,
        headers: { 'Content-Type': 'text/plain' },
      });
    }

    if (url.pathname !== '/v2') {
      return new Response('Not found. Use /v2?fen=...&depth=...\n', {
        status: 404,
        headers: { 'Content-Type': 'text/plain' },
      });
    }

    const fen = url.searchParams.get('fen');
    const depth = url.searchParams.get('depth') || '10';

    if (!fen) {
      return new Response('Missing required ?fen= parameter\n', {
        status: 400,
        headers: { 'Content-Type': 'text/plain' },
      });
    }

    // Validate depth: 1-15 only
    const depthInt = parseInt(depth, 10);
    if (isNaN(depthInt) || depthInt < 1 || depthInt > 15) {
      return new Response('depth must be 1..15\n', {
        status: 400,
        headers: { 'Content-Type': 'text/plain' },
      });
    }

    // Forward to upstream Stockfish API
    const upstreamUrl = `${UPSTREAM}?fen=${encodeURIComponent(fen)}&depth=${depthInt}`;

    try {
      const upstreamResp = await fetch(upstreamUrl, {
        method: 'GET',
        headers: {
          'User-Agent': 'OpenChess-Proxy/1.0 (Cloudflare-Worker)',
          'Accept': 'application/json',
        },
        // Cloudflare Workers have a 30s wall clock per request; the upstream
        // typically responds in <2s but heavy depths can take 10-15s.
        cf: { cacheTtl: 0 }, // never cache - chess moves are unique
      });

      if (!upstreamResp.ok) {
        return new Response(
          JSON.stringify({
            success: false,
            error: `upstream returned HTTP ${upstreamResp.status}`,
          }),
          {
            status: 502,
            headers: { 'Content-Type': 'application/json' },
          }
        );
      }

      // Return ONLY the JSON body (strip Cloudflare's response headers; the
      // board's parser doesn't need Nel:, Report-To:, CF-RAY:, etc).
      const body = await upstreamResp.text();
      return new Response(body, {
        status: 200,
        headers: {
          'Content-Type': 'application/json',
          // Tight CORS in case anyone wants to call this from a browser too
          'Access-Control-Allow-Origin': '*',
          // Force connection: close so the NINA's read loop sees EOF
          // promptly instead of waiting for keep-alive timeout.
          'Connection': 'close',
        },
      });
    } catch (err) {
      return new Response(
        JSON.stringify({
          success: false,
          error: `proxy error: ${err.message}`,
        }),
        {
          status: 502,
          headers: { 'Content-Type': 'application/json' },
        }
      );
    }
  },
};
