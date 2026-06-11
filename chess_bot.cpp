#include "chess_bot.h"
#include <Arduino.h>

ChessBot::ChessBot(BoardDriver* boardDriver, ChessEngine* chessEngine, BotDifficulty diff) {
    _boardDriver = boardDriver;
    _chessEngine = chessEngine;
    difficulty = diff;
    
    // Set difficulty settings
    switch(difficulty) {
        case BOT_EASY: settings = StockfishSettings::easy(); break;
        case BOT_MEDIUM: settings = StockfishSettings::medium(); break;
        case BOT_HARD: settings = StockfishSettings::hard(); break;
        case BOT_EXPERT: settings = StockfishSettings::expert(); break;
    }
    
    isWhiteTurn = true;
    gameStarted = false;
    botThinking = false;
    wifiConnected = false;
}

void ChessBot::begin() {
    Serial.println("=== Starting Chess Bot Mode ===");
    Serial.print("Bot Difficulty: ");
    
    switch(difficulty) {
        case BOT_EASY: Serial.println("Easy (Depth 6)"); break;
        case BOT_MEDIUM: Serial.println("Medium (Depth 10)"); break;
        case BOT_HARD: Serial.println("Hard (Depth 14)"); break;
        case BOT_EXPERT: Serial.println("Expert (Depth 16)"); break;
    }
    
    _boardDriver->clearAllLEDs();
    _boardDriver->showLEDs();
    
    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    showConnectionStatus();
    
    if (connectToWiFi()) {
        Serial.println("WiFi connected! Bot mode ready.");
        wifiConnected = true;
        
        // Show success animation
        for (int i = 0; i < 3; i++) {
            _boardDriver->clearAllLEDs();
            _boardDriver->showLEDs();
            delay(200);
            
            // Light up entire board green briefly
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    _boardDriver->setSquareLED(row, col, 0, 255, 0); // Green
                }
            }
            _boardDriver->showLEDs();
            delay(200);
        }
        
        initializeBoard();
        waitForBoardSetup();
    } else {
        Serial.println("Failed to connect to WiFi. Bot mode unavailable.");
        wifiConnected = false;
        
        // Show error animation (red flashing)
        for (int i = 0; i < 5; i++) {
            _boardDriver->clearAllLEDs();
            _boardDriver->showLEDs();
            delay(300);
            
            // Light up entire board red briefly
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    _boardDriver->setSquareLED(row, col, 255, 0, 0); // Red
                }
            }
            _boardDriver->showLEDs();
            delay(300);
        }
        
        _boardDriver->clearAllLEDs();
        _boardDriver->showLEDs();
    }
}

void ChessBot::update() {
    if (!wifiConnected) {
        return; // No WiFi, can't play against bot
    }
    
    if (!gameStarted) {
        return; // Waiting for initial setup
    }
    
    if (botThinking) {
        showBotThinking();
        return;
    }
    
    _boardDriver->readSensors();
    
    // Detect piece movements (player's turn - White pieces only)
    if (isWhiteTurn) {
        static unsigned long lastTurnDebug = 0;
        if (millis() - lastTurnDebug > 5000) {
            Serial.println("Your turn! Move a WHITE piece (uppercase letters)");
            lastTurnDebug = millis();
        }
        // Look for piece pickups and placements
        static int selectedRow = -1, selectedCol = -1;
        static bool piecePickedUp = false;

        // Breathe the player's back rank (rank 1) in soft white while it's
        // your turn and no piece is lifted yet, so you always know it's on
        // you to move (mirrors the bot's blue breathing on rank 8).
        if (!piecePickedUp) {
            _boardDriver->breatheRow(0, 0, 0, 0, 200); // white channel
        }

        // Check for piece pickup
        if (!piecePickedUp) {
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    if (!_boardDriver->getSensorState(row, col) && _boardDriver->getSensorPrev(row, col)) {
                        // Check what piece was picked up
                        char piece = board[row][col];
                        
                        if (piece != ' ') {
                            // Player should only be able to move White pieces (uppercase)
                            if (piece >= 'A' && piece <= 'Z') {
                            selectedRow = row;
                            selectedCol = col;
                            piecePickedUp = true;
                            
                            Serial.print("Player picked up WHITE piece '");
                            Serial.print(board[row][col]);
                            Serial.print("' at ");
                            Serial.print((char)('a' + col));
                            Serial.print(row + 1);
                            Serial.print(" (array position ");
                            Serial.print(row);
                            Serial.print(",");
                            Serial.print(col);
                            Serial.println(")");
                            
                            // Clear the turn-indicator breathing on rank 1
                            // first, otherwise its lit row bleeds into the
                            // move display (looks like extra lights).
                            _boardDriver->clearAllLEDs();

                            // Show selected square
                            _boardDriver->setSquareLED(row, col, 255, 0, 0); // Red
                            
                            // Show legal moves (includes castling + en-passant)
                            int moveCount = 0;
                            int moves[MAX_MOVES_PER_PIECE][2];
                            _chessEngine->getLegalMoves(board, &state, row, col, moveCount, moves);
                            
                            for (int i = 0; i < moveCount; i++) {
                                _boardDriver->setSquareLED(moves[i][0], moves[i][1], 255, 255, 255); // White
                            }
                            _boardDriver->showLEDs();
                            break;
                            } else {
                                // Player tried to pick up a Black piece - not allowed!
                                Serial.print("ERROR: You tried to pick up BLACK piece '");
                                Serial.print(piece);
                                Serial.print("' at ");
                                Serial.print((char)('a' + col));
                                Serial.print(row + 1);
                                Serial.println(". You can only move WHITE pieces!");
                                
                                // Flash red to indicate error
                                _boardDriver->blinkSquare(row, col, 3);
                            }
                        }
                    }
                }
                if (piecePickedUp) break;
            }
        }
        
        // Check for piece placement
        if (piecePickedUp) {
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    if (_boardDriver->getSensorState(row, col) && !_boardDriver->getSensorPrev(row, col)) {
                        // Check if piece was returned to its original position
                        if (row == selectedRow && col == selectedCol) {
                            // Piece returned to original position - cancel selection
                            Serial.println("Piece returned to original position. Selection cancelled.");
                            piecePickedUp = false;
                            selectedRow = selectedCol = -1;
                            
                            // Clear all indicators
                            _boardDriver->clearAllLEDs();
                            _boardDriver->showLEDs();
                            break;
                        }
                        
                        // Piece placed somewhere else - validate move
                        int moveCount = 0;
                        int moves[MAX_MOVES_PER_PIECE][2];
                        _chessEngine->getLegalMoves(board, &state, selectedRow, selectedCol, moveCount, moves);
                        
                        bool validMove = false;
                        for (int i = 0; i < moveCount; i++) {
                            if (moves[i][0] == row && moves[i][1] == col) {
                                validMove = true;
                                break;
                            }
                        }
                        
                        if (validMove) {
                            char piece = board[selectedRow][selectedCol];
                            
                            // Complete LED animations BEFORE API request
                            processPlayerMove(selectedRow, selectedCol, row, col, piece);
                            
                            // Flash confirmation on destination square for player move
                            confirmSquareCompletion(row, col);
                            
                            piecePickedUp = false;
                            selectedRow = selectedCol = -1;
                            
                            // Switch to bot's turn
                            isWhiteTurn = false;
                            botThinking = true;
                            
                            Serial.println("Player move completed. Bot thinking...");
                            
                            // Start bot move calculation
                            makeBotMove();
                        } else {
                            Serial.println("Invalid move! Please try again.");
                            _boardDriver->blinkSquare(row, col, 3); // Blink red for invalid move
                            
                            // Restore move indicators - piece is still selected
                            _boardDriver->clearAllLEDs();
                            
                            // Show selected square again
                            _boardDriver->setSquareLED(selectedRow, selectedCol, 255, 0, 0); // Red
                            
                            // Show possible moves again
                            int moveCount = 0;
                            int moves[MAX_MOVES_PER_PIECE][2];
                            _chessEngine->getLegalMoves(board, &state, selectedRow, selectedCol, moveCount, moves);
                            
                            for (int i = 0; i < moveCount; i++) {
                                _boardDriver->setSquareLED(moves[i][0], moves[i][1], 255, 255, 255); // White
                            }
                            _boardDriver->showLEDs();
                            
                            Serial.println("Piece is still selected. Place it on a valid move or return it to its original position.");
                        }
                        break;
                    }
                }
            }
        }
    }
    
    _boardDriver->updateSensorPrev();
}

bool ChessBot::connectToWiFi() {
    // Check for WiFi module
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("WiFi module not found!");
        return false;
    }
    
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        WiFi.begin(SECRET_SSID, SECRET_PASS);
        delay(5000);
        attempts++;
        
        Serial.print("Connection attempt ");
        Serial.print(attempts);
        Serial.print("/10 - Status: ");
        Serial.println(WiFi.status());
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("Failed to connect to WiFi");
        return false;
    }
}

bool ChessBot::ensureWiFiConnected() {
    // Fast path: still connected, nothing to do.
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.print("WiFi link DOWN (status=");
    Serial.print(WiFi.status());
    Serial.println("). Attempting quick reconnect...");

    // One quick attempt -- 8 second wait. The full connectToWiFi() does
    // 10x5s = 50s which is too aggressive for in-game recovery; the heavy
    // recovery is in reinitWiFiModule() called from the retry loop.
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 8000)) {
        delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi reconnected. IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    Serial.println("Quick WiFi reconnect failed.");
    return false;
}

bool ChessBot::reinitWiFiModule() {
    // Full NINA-W102 reset cycle. Drains any wedged internal state in
    // the WiFi co-processor, then re-establishes the link from scratch.
    // Slow (~12 s worst case) so only used as a last resort inside the
    // makeBotMove retry loop after multiple consecutive failures.
    Serial.println("Full WiFi module reset: disconnect -> end -> begin");
    WiFi.disconnect();
    delay(500);
    WiFi.end();
    delay(2000);

    WiFi.begin(SECRET_SSID, SECRET_PASS);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 10000)) {
        delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi module recovered. IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    Serial.println("WiFi module reset failed.");
    return false;
}

void ChessBot::showRetryFeedback(int attempt) {
    // Orange/amber pulse on rank 8 to indicate "actively recovering, hang
    // tight". Different colour from the blue thinking pulse so the user
    // can distinguish a normal slow API call from a retry. Number of
    // pulses scales with attempt count: 1 pulse for backoff before
    // attempt 2, 2 pulses before 3, etc -- so user gets visual sense of
    // how many retries have happened.
    int pulseCount = attempt; // attempt is the prior attempt number
    if (pulseCount < 1) pulseCount = 1;
    if (pulseCount > 5) pulseCount = 5;
    for (int p = 0; p < pulseCount; p++) {
        for (int c = 0; c < 8; c++) _boardDriver->setSquareLED(7, c, 255, 100, 0); // amber
        _boardDriver->showLEDs();
        delay(100);
        _boardDriver->clearAllLEDs();
        _boardDriver->showLEDs();
        delay(100);
    }
}

String ChessBot::makeStockfishRequest(String fen) {
    // Plain HTTP via Cloudflare Worker proxy. NINA-W102 TLS is unreliable;
    // see arduino_secrets.h header comment for the full reasoning.
    WiFiClient client;

    unsigned long t0 = millis();  // request start

    Serial.println("Making API request to Stockfish...");
    Serial.print("FEN: ");
    Serial.println(fen);

    // Pre-flight: WiFi must be up. If the AP rebooted or signal blipped,
    // a TLS handshake will hang forever instead of failing fast. Detect
    // and short-circuit so the retry loop in makeBotMove can decide
    // whether to do a full WiFi.end()+begin() recovery.
    int wifiStatus = WiFi.status();
    int rssi = WiFi.RSSI();
    Serial.print("[DBG] Pre-flight: WiFi status=");
    Serial.print(wifiStatus);
    Serial.print(" RSSI=");
    Serial.print(rssi);
    Serial.print(" dBm IP=");
    Serial.println(WiFi.localIP());
    if (wifiStatus != WL_CONNECTED) {
        Serial.print("PRE-FLIGHT FAIL: WiFi not connected (status=");
        Serial.print(wifiStatus);
        Serial.println(")");
        return "";
    }

    // Belt-and-suspenders: WiFiClient is stack-allocated and goes out
    // of scope at end of this function, but the underlying NINA-W102 socket
    // can linger in TIME_WAIT for several seconds and starve subsequent
    // connect() calls. Force a clean state before reusing the slot.
    client.stop();
    delay(50);

    // Render an initial guaranteed-visible thinking pulse BEFORE we open the
    // SSL socket. The Stockfish API typically responds in <1 s, which means
    // the per-frame breathing call inside the read loop only manages 2-3
    // frames -- not enough for the user to clearly see "the bot is thinking".
    // Drawing 5 pulse frames here (~600 ms) before client.connect() ensures
    // the user always gets visual confirmation that the request was sent,
    // independent of how fast the API responds. We force-render each frame
    // (no bucket skip) since this is pre-TLS so interrupt timing doesn't
    // matter yet.
    {
        const uint8_t brights[5] = {60, 130, 200, 130, 60};
        for (int f = 0; f < 5; f++) {
            _boardDriver->clearAllLEDs();
            for (int c = 0; c < 8; c++) {
                _boardDriver->setSquareLED(7, c, 0, 0, brights[f]);
            }
            _boardDriver->showLEDs();
            delay(120);
        }
    }

    Serial.print("[DBG] Calling client.connect(");
    Serial.print(STOCKFISH_API_URL);
    Serial.print(":");
    Serial.print(STOCKFISH_API_PORT);
    Serial.println(")...");
    unsigned long tConnect = millis();
    bool connected = client.connect(STOCKFISH_API_URL, STOCKFISH_API_PORT);
    unsigned long connectMs = millis() - tConnect;
    Serial.print("[DBG] connect() returned ");
    Serial.print(connected ? "TRUE" : "FALSE");
    Serial.print(" after ");
    Serial.print(connectMs);
    Serial.println("ms");

    if (!connected) {
        Serial.print("CONNECT FAIL: TCP connect to ");
        Serial.print(STOCKFISH_API_URL);
        Serial.print(":");
        Serial.print(STOCKFISH_API_PORT);
        Serial.println(" failed");
        client.stop();
        delay(100);
        return "";
    }

    // URL encode the FEN string
    String encodedFen = urlEncode(fen);

    // Build entire HTTP request in ONE buffer so we send it as a single
    // chunk. Multiple separate client.println() calls can race against
    // the NINA's TCP send buffer and (rarely) result in partial / out-of-
    // order writes that Cloudflare interprets as a malformed request and
    // silently drops -- which is exactly the "READ FAIL: no response after
    // 5s" symptom we see.
    String request;
    request.reserve(512);
    request += "GET ";
    request += STOCKFISH_API_PATH;
    request += "?fen=";
    request += encodedFen;
    request += "&depth=";
    request += String(settings.depth);
    request += " HTTP/1.1\r\n";
    request += "Host: ";
    request += STOCKFISH_API_URL;
    request += "\r\n";
    // Cloudflare is pickier than the origin: requests without a User-Agent
    // are sometimes treated as suspicious and either silently dropped or
    // sent to a slow challenge path. Always include a sensible UA.
    request += "User-Agent: OpenChess/1.2.2 (Arduino-NINA)\r\n";
    request += "Accept: application/json\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";

    Serial.print("[DBG] Request URL: ");
    Serial.print(STOCKFISH_API_PATH);
    Serial.print("?fen=");
    Serial.print(encodedFen);
    Serial.print("&depth=");
    Serial.println(settings.depth);
    Serial.print("[DBG] Request size: ");
    Serial.print(request.length());
    Serial.println(" bytes");

    unsigned long tWrite = millis();
    size_t written = client.print(request);
    client.flush(); // force NINA to push the buffer out NOW
    unsigned long writeMs = millis() - tWrite;
    Serial.print("[DBG] Wrote ");
    Serial.print(written);
    Serial.print("/");
    Serial.print(request.length());
    Serial.print(" bytes in ");
    Serial.print(writeMs);
    Serial.println("ms");

    if (written < request.length()) {
        Serial.println("WRITE FAIL: NINA short-write -- request body incomplete");
        client.stop();
        delay(100);
        return "";
    }

    // Bounded read loop. Render the breathing 'thinking' indicator
    // very sparingly (only when the brightness bucket changes -- about
    // 5 times per second on average). Higher-rate NeoPixel updates
    // disable interrupts for long enough to corrupt the concurrent
    // WiFiSSL handshake / read.
    //
    // Strict bytewise read with hard cap (instead of client.readString())
    // to avoid edge case where the server stalls mid-stream and the
    // Arduino Stream timeout extends indefinitely.
    String response;
    response.reserve(2048);
    unsigned long startTime = millis();
    unsigned long lastByteTime = millis();
    unsigned long firstByteMs = 0;
    // 8s gives Cloudflare's cold path a chance. We see 99%+ first-byte
    // latency under 1.5s when the path is warm; cold path can be 4-6s.
    const unsigned long INTER_BYTE_TIMEOUT_MS = 8000;
    while (millis() - startTime < settings.timeoutMs) {
        // Check connection health AND data availability separately
        int avail = client.available();
        if (avail > 0) {
            if (firstByteMs == 0) {
                firstByteMs = millis() - tWrite;
                Serial.print("[DBG] First byte after ");
                Serial.print(firstByteMs);
                Serial.println("ms");
            }
            while (client.available()) {
                char c = client.read();
                response += c;
                if (response.length() >= 4096) break; // hard cap (avoid OOM)
            }
            lastByteTime = millis();
            if (response.length() >= 4096) break;
        } else if (!client.connected()) {
            // Server closed the connection. If we have bytes, we're done.
            if (response.length() > 0) break;
            // No bytes and connection closed = empty response (server hung up)
            Serial.print("READ FAIL: server closed connection without sending data after ");
            Serial.print(millis() - tWrite);
            Serial.println("ms");
            client.stop();
            delay(100);
            return "";
        } else if (millis() - lastByteTime > INTER_BYTE_TIMEOUT_MS && response.length() == 0) {
            // No response at all after 8s: server is hung
            Serial.print("READ FAIL: no response from server after ");
            Serial.print(INTER_BYTE_TIMEOUT_MS);
            Serial.print("ms (connect was OK, request sent ");
            Serial.print(written);
            Serial.println(" bytes)");
            client.stop();
            delay(100);
            return "";
        }
        showBotThinking();
        delay(40);
    }

    // ALWAYS clean up the socket
    client.stop();
    delay(100);
    _boardDriver->clearAllLEDs();
    _boardDriver->showLEDs();

    if (response.length() == 0) {
        Serial.print("READ FAIL: timed out after ");
        Serial.print(settings.timeoutMs);
        Serial.println("ms with no response");
        return "";
    }

    Serial.print("[DBG] Total request time: ");
    Serial.print(millis() - t0);
    Serial.print("ms (response ");
    Serial.print(response.length());
    Serial.println(" bytes)");

    // Debug: Print raw response
    Serial.println("=== RAW API RESPONSE ===");
    Serial.println(response);
    Serial.println("=== END RAW RESPONSE ===");

    return response;
}

bool ChessBot::parseStockfishResponse(String response, String &bestMove) {
    // Split HTTP headers from the body first. Cloudflare adds headers whose
    // VALUES are themselves JSON (e.g. "Report-To: {...}", "Nel: {...}"), so
    // a naive indexOf("{") on the whole response grabs the wrong object. The
    // real payload starts after the blank line that ends the headers.
    String body = response;
    int headerEnd = response.indexOf("\r\n\r\n");
    if (headerEnd != -1) {
        body = response.substring(headerEnd + 4);
    } else {
        headerEnd = response.indexOf("\n\n");
        if (headerEnd != -1) body = response.substring(headerEnd + 2);
    }

    // Find JSON content in the body.
    int jsonStart = body.indexOf("{");
    if (jsonStart == -1) {
        Serial.println("No JSON found in response");
        return false;
    }

    String json = body.substring(jsonStart);
    Serial.print("Extracted JSON: ");
    Serial.println(json);
    
    // Check if request was successful
    if (json.indexOf("\"success\":true") == -1) {
        Serial.println("API request was not successful");
        return false;
    }
    
    // Parse bestmove field - format: "bestmove":"bestmove b7b6 ponder f3e5"
    int bestmoveStart = json.indexOf("\"bestmove\":\"");
    if (bestmoveStart == -1) {
        Serial.println("No bestmove found in response");
        return false;
    }
    
    bestmoveStart += 12; // Skip "bestmove":"
    int bestmoveEnd = json.indexOf("\"", bestmoveStart);
    if (bestmoveEnd == -1) {
        Serial.println("Invalid bestmove format");
        return false;
    }
    
    String fullMove = json.substring(bestmoveStart, bestmoveEnd);
    Serial.print("Full move string: ");
    Serial.println(fullMove);
    
    // Extract just the move part after "bestmove " and before " ponder"
    int moveStart = fullMove.indexOf("bestmove ");
    if (moveStart == -1) {
        Serial.println("No 'bestmove' prefix found");
        return false;
    }
    
    moveStart += 9; // Skip "bestmove "
    int moveEnd = fullMove.indexOf(" ", moveStart);
    if (moveEnd == -1) {
        // No ponder part, take rest of string
        bestMove = fullMove.substring(moveStart);
    } else {
        bestMove = fullMove.substring(moveStart, moveEnd);
    }
    
    Serial.print("Parsed move: ");
    Serial.println(bestMove);
    
    return bestMove.length() >= 4;
}

void ChessBot::makeBotMove() {
    Serial.println("=== BOT MOVE CALCULATION ===");
    Serial.print("Bot is playing as: ");
    Serial.println(isWhiteTurn ? "White" : "Black");
    Serial.print("Current board state (FEN): ");

    String fen = boardToFEN();

    // Defense in depth against WiFiNINA TLS flakiness:
    //   * 5 attempts max
    //   * Exponential backoff between attempts (500/1000/2000/4000/6000 ms)
    //   * Pre-flight WiFi.status() check; quick reconnect if disconnected
    //   * After attempt 3 still failing -> full WiFi.end()/begin() reset
    //     of the NINA module (covers cases where the NINA-W102 internal
    //     state is wedged but the network is otherwise fine)
    //   * Visual orange retry pulse during backoff (so user knows the
    //     board is actively recovering, not frozen)
    //
    // Total worst case: ~3*timeoutMs + 13s of backoff + 12s of WiFi reset
    // ~= 100s. In practice attempt 1 succeeds 95%+ of the time and the
    // user never sees a retry. When they do trigger, attempt 2 typically
    // works.
    String response;
    const int MAX_ATTEMPTS = 5;
    const unsigned long backoffMs[MAX_ATTEMPTS] = {0, 500, 1000, 2000, 4000};

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        // Backoff (skipped on first attempt)
        if (attempt > 1) {
            Serial.print("Backoff ");
            Serial.print(backoffMs[attempt - 1]);
            Serial.print("ms before attempt ");
            Serial.print(attempt);
            Serial.println("...");
            showRetryFeedback(attempt - 1);
            delay(backoffMs[attempt - 1]);
        }

        // After 3 consecutive failures, the NINA module is probably wedged.
        // Do a full reset before the next attempt.
        if (attempt == 4) {
            Serial.println("3 failures in a row -- resetting WiFi module...");
            if (!reinitWiFiModule()) {
                Serial.println("WiFi reinit failed -- continuing attempts anyway");
            }
        }

        // Make sure WiFi is up before each attempt
        if (!ensureWiFiConnected()) {
            Serial.print("WiFi recovery failed on attempt ");
            Serial.println(attempt);
            continue;
        }

        Serial.print("--- Stockfish API attempt ");
        Serial.print(attempt);
        Serial.print("/");
        Serial.print(MAX_ATTEMPTS);
        Serial.println(" ---");

        response = makeStockfishRequest(fen);
        if (response.length() > 0) {
            if (attempt > 1) {
                Serial.print("RECOVERED: attempt ");
                Serial.print(attempt);
                Serial.println(" succeeded.");
            }
            break;
        }
        Serial.print("Attempt ");
        Serial.print(attempt);
        Serial.println(" returned empty response.");
    }

    if (response.length() > 0) {
        String bestMove;
        if (parseStockfishResponse(response, bestMove)) {
            int fromRow, fromCol, toRow, toCol;
            if (parseMove(bestMove, fromRow, fromCol, toRow, toCol)) {
                Serial.print("Bot move: ");
                Serial.println(bestMove);

                executeBotMove(fromRow, fromCol, toRow, toCol);

                // Switch back to player's turn
                isWhiteTurn = true;
                botThinking = false;

                Serial.println("Bot move completed. Your turn!");
            } else {
                Serial.println("Failed to parse bot move - returning turn to player");
                botThinking = false;
                isWhiteTurn = true;
            }
        } else {
            Serial.println("Failed to parse Stockfish response - returning turn to player");
            botThinking = false;
            isWhiteTurn = true;
        }
    } else {
        // ALL retries exhausted. Give the turn back so the user isn't
        // stranded. Almost always means stockfish.online or the local
        // network is genuinely down; the next player move will trigger
        // a fresh attempt cycle.
        Serial.println("=== ALL 5 ATTEMPTS FAILED ===");
        Serial.println("Returning turn to player. Try moving again or check WiFi.");
        botThinking = false;
        isWhiteTurn = true;

        // Visual feedback: red flash on rank 8 to signal exhausted retries.
        for (int i = 0; i < 3; i++) {
            for (int c = 0; c < 8; c++) _boardDriver->setSquareLED(7, c, 200, 0, 0);
            _boardDriver->showLEDs();
            delay(150);
            _boardDriver->clearAllLEDs();
            _boardDriver->showLEDs();
            delay(150);
        }
    }
}

String ChessBot::boardToFEN() {
    String fen = "";
    
    // Board position - FEN expects rank 8 (black pieces) first, rank 1 (white pieces) last
    // Our array: row 0 = white pieces, row 7 = black pieces
    // So we need to reverse the order: start from row 7 and go to row 0
    for (int row = 7; row >= 0; row--) {
        int emptyCount = 0;
        for (int col = 0; col < 8; col++) {
            if (board[row][col] == ' ') {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen += String(emptyCount);
                    emptyCount = 0;
                }
                fen += board[row][col];
            }
        }
        if (emptyCount > 0) {
            fen += String(emptyCount);
        }
        if (row > 0) fen += "/";
    }
    
    // Active color - when we call this, it's the bot's turn (Black)
    // But we need to tell Stockfish whose turn it actually is
    fen += isWhiteTurn ? " w" : " b";
    
    Serial.print("Current turn in FEN: ");
    Serial.println(isWhiteTurn ? "White (w)" : "Black (b)");

    // Castling availability - derive from the live game state. A hard-coded
    // "KQkq" produces an ILLEGAL FEN once anyone has castled or moved a
    // king/rook (Stockfish then rejects it with "Invalid FEN"), which used
    // to silently bounce the turn back to the player.
    String castle = "";
    if (state.whiteCanCastleKingside)  castle += "K";
    if (state.whiteCanCastleQueenside) castle += "Q";
    if (state.blackCanCastleKingside)  castle += "k";
    if (state.blackCanCastleQueenside) castle += "q";
    if (castle.length() == 0) castle = "-";
    fen += " " + castle;

    // En passant target square. Use the live state if a pawn just made a
    // two-square advance; otherwise "-".
    if (state.enPassantRow >= 0 && state.enPassantCol >= 0) {
        fen += " ";
        fen += (char)('a' + state.enPassantCol);
        fen += (char)('1' + state.enPassantRow);
    } else {
        fen += " -";
    }

    // Halfmove clock (for the 50-move rule).
    fen += " " + String(state.halfMoveClock);
    
    // Fullmove number (simplified)
    fen += " 1";
    
    Serial.print("Generated FEN: ");
    Serial.println(fen);
    
    return fen;
}

bool ChessBot::parseMove(String move, int &fromRow, int &fromCol, int &toRow, int &toCol) {
    if (move.length() < 4) return false;
    
    fromCol = move.charAt(0) - 'a';
    fromRow = (move.charAt(1) - '0') - 1;  // Convert 1-8 to 0-7 (1=row 0, 8=row 7)
    toCol = move.charAt(2) - 'a';
    toRow = (move.charAt(3) - '0') - 1;    // Convert 1-8 to 0-7
    
    // Debug coordinate conversion
    Serial.print("Move string: ");
    Serial.println(move);
    Serial.print("Parsed coordinates: (");
    Serial.print(fromRow);
    Serial.print(",");
    Serial.print(fromCol);
    Serial.print(") to (");
    Serial.print(toRow);
    Serial.print(",");
    Serial.print(toCol);
    Serial.println(")");
    Serial.print("In chess notation: ");
    Serial.print((char)('a' + fromCol));
    Serial.print(fromRow + 1);
    Serial.print(" to ");
    Serial.print((char)('a' + toCol));
    Serial.print(toRow + 1);
    
    // Check for promotion
    if (move.length() >= 5) {
        char promotionPiece = move.charAt(4);
        Serial.print(" (promotes to ");
        Serial.print(promotionPiece);
        Serial.print(")");
    }
    Serial.println();
    
    return (fromRow >= 0 && fromRow < 8 && fromCol >= 0 && fromCol < 8 &&
            toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8);
}

void ChessBot::executeBotMove(int fromRow, int fromCol, int toRow, int toCol) {
    char piece = board[fromRow][fromCol];
    char capturedPiece = board[toRow][toCol];

    // Detect castling before mutating the board.
    bool isCastle = (piece == 'K' || piece == 'k') &&
                    (fromRow == toRow) && (fromCol == 4) &&
                    (toCol == 6 || toCol == 2);

    // Update board state (king)
    board[toRow][toCol] = piece;
    board[fromRow][fromCol] = ' ';
    
    Serial.print("Bot wants to move piece from ");
    Serial.print((char)('a' + fromCol));
    Serial.print(fromRow + 1);
    Serial.print(" to ");
    Serial.print((char)('a' + toCol));
    Serial.println(toRow + 1);
    Serial.println("Please make this move on the physical board...");
    
    // Show the move that needs to be made
    showBotMoveIndicator(fromRow, fromCol, toRow, toCol);
    
    // Wait for user to physically complete the bot's move
    waitForBotMoveCompletion(fromRow, fromCol, toRow, toCol);
    
    if (capturedPiece != ' ') {
        Serial.print("Piece captured: ");
        Serial.println(capturedPiece);
        _boardDriver->captureAnimation();
    }

    // Confirm the king's destination, then (if castling) move the rook too:
    // update it internally and show a blinking-blue source + solid-blue dest
    // so the user knows exactly which rook to slide where.
    confirmSquareCompletion(toRow, toCol);

    if (isCastle) {
        int rookFromCol = (toCol == 6) ? 7 : 0;
        int rookToCol   = (toCol == 6) ? 5 : 3;
        char rook = board[toRow][rookFromCol];
        board[toRow][rookToCol] = rook;
        board[toRow][rookFromCol] = ' ';

        Serial.print("Bot castled: move the rook from ");
        Serial.print((char)('a' + rookFromCol));
        Serial.print(toRow + 1);
        Serial.print(" to ");
        Serial.print((char)('a' + rookToCol));
        Serial.println(toRow + 1);

        _boardDriver->readSensors();
        unsigned long start = millis();
        bool placed = false;
        while (!placed && millis() - start < 30000) {
            for (int f = 0; f < 2; f++) {
                _boardDriver->clearAllLEDs();
                _boardDriver->setSquareLED(toRow, rookToCol, 0, 0, 255);     // rook dest
                if (f == 0) {
                    _boardDriver->setSquareLED(toRow, rookFromCol, 0, 0, 255); // rook src blink
                }
                _boardDriver->showLEDs();
                delay(300);
                _boardDriver->readSensors();
                if (_boardDriver->getSensorState(toRow, rookToCol)) {
                    placed = true;
                    break;
                }
            }
        }
        _boardDriver->clearAllLEDs();
        _boardDriver->showLEDs();
        _boardDriver->readSensors();
        _boardDriver->updateSensorPrev();
    }

    // Keep castling rights current for the bot side too.
    updateCastlingRights(piece, fromRow, fromCol);

    Serial.println("Bot move completed. Your turn!");
}

void ChessBot::updateCastlingRights(char piece, int fromRow, int fromCol) {
    // A king move forfeits both of that side's castling rights; a rook move
    // off its home square forfeits that side's right. This mirrors the rules
    // ChessEngine::applyMove applies in Human-vs-Human mode.
    if (piece == 'K') {
        state.whiteCanCastleKingside = false;
        state.whiteCanCastleQueenside = false;
    } else if (piece == 'k') {
        state.blackCanCastleKingside = false;
        state.blackCanCastleQueenside = false;
    } else if (piece == 'R' || piece == 'r') {
        if (fromRow == 0 && fromCol == 0) state.whiteCanCastleQueenside = false;
        if (fromRow == 0 && fromCol == 7) state.whiteCanCastleKingside = false;
        if (fromRow == 7 && fromCol == 0) state.blackCanCastleQueenside = false;
        if (fromRow == 7 && fromCol == 7) state.blackCanCastleKingside = false;
    }
}

void ChessBot::showBotThinking() {
    // Slow 'breathing' on rank 8 (the bot's back rank). Sine-wave brightness
    // with ~1.5 s period. We only push to the LED strip when the brightness
    // bucket actually changes (8 discrete steps), so the average frame rate
    // is ~5 fps -- low enough that the NeoPixel show() (~2.5 ms with
    // interrupts off) doesn't disrupt the concurrent WiFiSSL TLS handshake
    // / read in makeStockfishRequest.
    const float periodMs = 1500.0f;
    float phase = (millis() % (unsigned long)periodMs) / periodMs;
    // Sine 0..1 (rectified)
    float b = (sinf(phase * 2.0f * 3.14159f) + 1.0f) * 0.5f;
    int bucket = (int)(b * 8.0f);     // 0..7 discrete levels
    static int lastBucket = -1;
    if (bucket == lastBucket) return; // skip redundant frames
    lastBucket = bucket;

    uint8_t bright = 30 + bucket * 25; // 30..205 range, never fully off
    _boardDriver->clearAllLEDs();
    for (int c = 0; c < 8; c++) {
        _boardDriver->setSquareLED(7, c, 0, 0, bright); // pure blue rank 8
    }
    _boardDriver->showLEDs();
}

void ChessBot::showConnectionStatus() {
    // Show WiFi connection attempt with animated LEDs
    for (int i = 0; i < 8; i++) {
        _boardDriver->setSquareLED(3, i, 0, 0, 255); // Blue row
        _boardDriver->showLEDs();
        delay(200);
    }
}

void ChessBot::initializeBoard() {
    // Copy initial board state
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            board[row][col] = INITIAL_BOARD[row][col];
        }
    }
    // Fresh game: restore all castling rights / clear en-passant.
    state.reset();
}

void ChessBot::waitForBoardSetup() {
    Serial.println("Please set up the chess board in starting position...");
    
    while (!_boardDriver->checkInitialBoard(INITIAL_BOARD)) {
        _boardDriver->readSensors();
        _boardDriver->updateSetupDisplay(INITIAL_BOARD);
        _boardDriver->showLEDs();
        delay(100);
    }
    
    Serial.println("Board setup complete! Game starting...");
    _boardDriver->fireworkAnimation();
    gameStarted = true;
    
    // Show initial board state
    printCurrentBoard();
}

void ChessBot::processPlayerMove(int fromRow, int fromCol, int toRow, int toCol, char piece) {
    char capturedPiece = board[toRow][toCol];

    // Detect castling BEFORE mutating the board: the king moves two files
    // along its own rank (e1->g1 / e1->c1, or e8->g8 / e8->c8).
    bool isCastle = (piece == 'K' || piece == 'k') &&
                    (fromRow == toRow) && (fromCol == 4) &&
                    (toCol == 6 || toCol == 2);

    // Update board state (king)
    board[toRow][toCol] = piece;
    board[fromRow][fromCol] = ' ';

    Serial.print("Player moved ");
    Serial.print(piece);
    Serial.print(" from ");
    Serial.print((char)('a' + fromCol));
    Serial.print(fromRow + 1);
    Serial.print(" to ");
    Serial.print((char)('a' + toCol));
    Serial.println(toRow + 1);

    if (capturedPiece != ' ') {
        Serial.print("Captured ");
        Serial.println(capturedPiece);
        _boardDriver->captureAnimation();
    }

    // Castling: move the rook internally and tell the user exactly which
    // rook to move where (a single blinking-blue source + solid-blue dest),
    // instead of leaving them to guess and lighting the rook's normal moves.
    if (isCastle) {
        int rookFromCol = (toCol == 6) ? 7 : 0;
        int rookToCol   = (toCol == 6) ? 5 : 3;
        char rook = board[toRow][rookFromCol];
        board[toRow][rookToCol] = rook;
        board[toRow][rookFromCol] = ' ';

        Serial.print("Castling: move the rook from ");
        Serial.print((char)('a' + rookFromCol));
        Serial.print(toRow + 1);
        Serial.print(" to ");
        Serial.print((char)('a' + rookToCol));
        Serial.println(toRow + 1);

        // Show the hint: king dest soft white, rook dest solid blue,
        // rook source blinking blue. Hold until the user actually places
        // the rook on its destination square.
        _boardDriver->readSensors();
        _boardDriver->updateSensorPrev();
        unsigned long start = millis();
        bool placed = false;
        while (!placed && millis() - start < 30000) {
            for (int f = 0; f < 2; f++) {
                _boardDriver->clearAllLEDs();
                _boardDriver->setSquareLED(toRow, toCol, 0, 0, 0, 100);     // king dest
                _boardDriver->setSquareLED(toRow, rookToCol, 0, 0, 255);    // rook dest
                if (f == 0) {
                    _boardDriver->setSquareLED(toRow, rookFromCol, 0, 0, 255); // rook src blink
                }
                _boardDriver->showLEDs();
                delay(300);
                _boardDriver->readSensors();
                // Done when the rook is sitting on its destination square.
                if (_boardDriver->getSensorState(toRow, rookToCol)) {
                    placed = true;
                    break;
                }
            }
        }
        _boardDriver->clearAllLEDs();
        _boardDriver->showLEDs();
        _boardDriver->readSensors();
        _boardDriver->updateSensorPrev();
    }

    // Check for pawn promotion
    if (_chessEngine->isPawnPromotion(piece, toRow)) {
        char promotedPiece = _chessEngine->getPromotedPiece(piece);
        board[toRow][toCol] = promotedPiece;
        Serial.print("Pawn promoted to ");
        Serial.println(promotedPiece);
        _boardDriver->promotionAnimation(toCol);
    }

    // Keep castling rights current so getLegalMoves won't offer an illegal
    // castle after the king or a rook has moved.
    updateCastlingRights(piece, fromRow, fromCol);
}

String ChessBot::urlEncode(String str) {
    String encoded = "";
    char c;
    char code0;
    char code1;
    
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == ' ') {
            encoded += "%20";
        } else if (c == '/') {
            encoded += "%2F";
        } else if (isalnum(c)) {
            encoded += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) {
                code0 = c - 10 + 'A';
            }
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }
    return encoded;
}

void ChessBot::showBotMoveIndicator(int fromRow, int fromCol, int toRow, int toCol) {
    // Clear all LEDs first
    _boardDriver->clearAllLEDs();
    
    // Show source square flashing (where to pick up from)
    _boardDriver->setSquareLED(fromRow, fromCol, 255, 255, 255); // White flashing
    
    // Show destination square solid (where to place)
    _boardDriver->setSquareLED(toRow, toCol, 255, 255, 255);     // White solid
    
    _boardDriver->showLEDs();
}

void ChessBot::waitForBotMoveCompletion(int fromRow, int fromCol, int toRow, int toCol) {
    bool piecePickedUp = false;
    bool moveCompleted = false;
    static unsigned long lastBlink = 0;
    static bool blinkState = false;
    
    Serial.println("Waiting for you to complete the bot's move...");
    
    while (!moveCompleted) {
        _boardDriver->readSensors();
        
        // Blink the source square
        if (millis() - lastBlink > 500) {
            _boardDriver->clearAllLEDs();
            if (blinkState && !piecePickedUp) {
                _boardDriver->setSquareLED(fromRow, fromCol, 255, 255, 255); // Flash source
            }
            _boardDriver->setSquareLED(toRow, toCol, 255, 255, 255);         // Always show destination
            _boardDriver->showLEDs();
            
            blinkState = !blinkState;
            lastBlink = millis();
        }
        
        // Check if piece was picked up from source
        if (!piecePickedUp && !_boardDriver->getSensorState(fromRow, fromCol)) {
            piecePickedUp = true;
            Serial.println("Bot piece picked up, now place it on the destination...");
            
            // Stop blinking source, just show destination
            _boardDriver->clearAllLEDs();
            _boardDriver->setSquareLED(toRow, toCol, 255, 255, 255);
            _boardDriver->showLEDs();
        }
        
        // Check if piece was placed on destination
        if (piecePickedUp && _boardDriver->getSensorState(toRow, toCol)) {
            moveCompleted = true;
            Serial.println("Bot move completed on physical board!");
        }
        
        delay(50);
        _boardDriver->updateSensorPrev();
    }
}

void ChessBot::confirmMoveCompletion() {
    // This will be called with specific square coordinates when we need them
    confirmSquareCompletion(-1, -1); // Default - no specific square
}

void ChessBot::confirmSquareCompletion(int row, int col) {
    if (row >= 0 && col >= 0) {
        // Flash specific square twice
        for (int flash = 0; flash < 2; flash++) {
            _boardDriver->setSquareLED(row, col, 0, 255, 0); // Green flash
            _boardDriver->showLEDs();
            delay(150);
            
            _boardDriver->clearAllLEDs();
            _boardDriver->showLEDs();
            delay(150);
        }
    } else {
        // Flash entire board (fallback for when we don't have specific coords)
        for (int flash = 0; flash < 2; flash++) {
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    _boardDriver->setSquareLED(r, c, 0, 255, 0); // Green flash
                }
            }
            _boardDriver->showLEDs();
            delay(150);
            
            _boardDriver->clearAllLEDs();
            _boardDriver->showLEDs();
            delay(150);
        }
    }
}

void ChessBot::printCurrentBoard() {
    Serial.println("=== CURRENT BOARD STATE ===");
    Serial.println("  a b c d e f g h");
    // Print rank 8 (row 7) at the top down to rank 1 (row 0) - standard view.
    for (int row = 7; row >= 0; row--) {
        Serial.print(row + 1);
        Serial.print(" ");
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (piece == ' ') {
                Serial.print(". ");
            } else {
                Serial.print(piece);
                Serial.print(" ");
            }
        }
        Serial.print(" ");
        Serial.println(row + 1);
    }
    Serial.println("  a b c d e f g h");
    Serial.println("White pieces (uppercase): R N B Q K P");
    Serial.println("Black pieces (lowercase): r n b q k p");
    Serial.println("========================");
}

void ChessBot::setDifficulty(BotDifficulty diff) {
    difficulty = diff;
    switch(difficulty) {
        case BOT_EASY: settings = StockfishSettings::easy(); break;
        case BOT_MEDIUM: settings = StockfishSettings::medium(); break;
        case BOT_HARD: settings = StockfishSettings::hard(); break;
        case BOT_EXPERT: settings = StockfishSettings::expert(); break;
    }
    
    Serial.print("Bot difficulty changed to: ");
    switch(difficulty) {
        case BOT_EASY: Serial.println("Easy"); break;
        case BOT_MEDIUM: Serial.println("Medium"); break;
        case BOT_HARD: Serial.println("Hard"); break;
        case BOT_EXPERT: Serial.println("Expert"); break;
    }
}