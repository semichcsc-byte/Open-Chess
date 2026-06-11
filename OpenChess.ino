#include "board_driver.h"
#include "chess_engine.h"
#include "chess_moves.h"
#include "sensor_test.h"
#include "chess_bot.h"
#include "game_persistence.h"

#include <math.h>

// Forward declaration: defined later in this file (next to the other
// game-selection helpers). Needed because the reset-gesture code in loop()
// references it before its definition.
extern const char STARTING_BOARD[8][8];

// Firmware version printed at boot for support / debugging.
#define OPENCHESS_FW_VERSION "1.4.0-rp2040"

// Set to 1 to print verbose DEBUG diagnostics on the serial monitor.
// 0 keeps the serial output clean: just the banner, game events, and
// useful play hints (no 10s "uptime" spam, no boot internals).
#define DEBUG_VERBOSE 0
#if DEBUG_VERBOSE
  #define DBG(x)   Serial.print(x)
  #define DBGLN(x) Serial.println(x)
#else
  #define DBG(x)
  #define DBGLN(x)
#endif

// Uncomment the next line to enable WiFi features (requires compatible board)
#define ENABLE_WIFI  // Currently disabled - RP2040 boards use local mode only
#ifdef ENABLE_WIFI
  // Use different WiFi manager based on board type
  #if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_NANO_RP2040_CONNECT)
    #include "wifi_manager.h"  // Full WiFi implementation for boards with WiFiNINA
  #else
    #include "wifi_manager_rp2040.h"  // Placeholder for RP2040 and other boards
    #define WiFiManager WiFiManagerRP2040
  #endif
#endif

// ---------------------------
// Game State and Configuration
// ---------------------------

// Game Mode Definitions
enum GameMode {
  MODE_SELECTION = 0,
  MODE_CHESS_MOVES = 1,
  MODE_CHESS_BOT = 2,      // Chess vs Bot mode
  MODE_GAME_3 = 3,         // Reserved for future game mode
  MODE_SENSOR_TEST = 4
};

// Global instances
BoardDriver boardDriver;
ChessEngine chessEngine;
ChessMoves chessMoves(&boardDriver, &chessEngine);
SensorTest sensorTest(&boardDriver);
ChessBot chessBot(&boardDriver, &chessEngine, BOT_MEDIUM);

#ifdef ENABLE_WIFI
WiFiManager wifiManager;
#endif

// Current game state
GameMode currentMode = MODE_SELECTION;
bool modeInitialized = false;

// ---------------------------
// Function Prototypes
// ---------------------------
void showGameSelection();
void handleGameSelection();
void initializeSelectedMode(GameMode mode);
BotDifficulty askBotDifficulty();

// ---------------------------
// SETUP
// ---------------------------
void setup() {
  // Initialize Serial with extended timeout
  Serial.begin(9600);
  
  // Wait for Serial to be ready (critical for RP2040)
  unsigned long startTime = millis();
  while (!Serial && (millis() - startTime < 10000)) {
    // Wait up to 10 seconds for Serial connection
    delay(100);
  }
  
  // Force a delay to ensure Serial is stable
  delay(2000);
  
  Serial.println();
  Serial.println("================================================");
  Serial.println("         OpenChess Starting Up");
  Serial.print  ("         Firmware: v");
  Serial.println(OPENCHESS_FW_VERSION);
  Serial.println("         Fork:    semichcsc-byte/Open-Chess");
  Serial.println("================================================");
  DBGLN("DEBUG: Serial communication established");

  // Initialize board driver
  boardDriver.begin();
  DBGLN("DEBUG: Board driver initialized");

  // Run the chess-engine self-tests before anything else. These are our
  // regression net for the rules code; results print to serial and, on any
  // failure, the whole board flashes red 5x so a broken build is impossible
  // to miss.
  int testFailures = chessEngine.runSelfTests();
  if (testFailures > 0) {
    Serial.print("WARNING: ");
    Serial.print(testFailures);
    Serial.println(" engine self-tests FAILED - re-flash a clean release!");
    for (int i = 0; i < 5; i++) {
      for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
          boardDriver.setSquareLED(r, c, 255, 0, 0, 0);
      boardDriver.showLEDs();
      delay(200);
      boardDriver.clearAllLEDs();
      boardDriver.showLEDs();
      delay(200);
    }
  }

#ifdef ENABLE_WIFI
  // Initialize WiFi Manager (AP for optional web selection; it is torn
  // down cleanly before station mode when AI mode connects to your router).
  wifiManager.begin();
  DBGLN("DEBUG: WiFi Manager initialized (AP: OpenChessBoard / chess123)");
#else
  DBGLN("DEBUG: Local mode only (WiFi disabled)");
#endif

  // Try to silently resume a saved game. It survives power-off and stays
  // active until the player puts all 32 pieces back on the starting squares
  // (the reset gesture in loop()). No resume menu, by design.
  persistenceBegin();
  SavedGame saved;
  if (persistenceLoadGame(saved)) {
    Serial.println("Saved game found - resuming where you left off.");
    if (saved.mode == 2) {
      currentMode = MODE_CHESS_BOT;
      chessBot.resumeFrom(saved);
    } else {
      currentMode = MODE_CHESS_MOVES;
      chessMoves.resumeFrom(saved);
    }
    modeInitialized = true; // skip the menu and piece-setup; resume directly
    Serial.println("================================================");
    return;
  }

  // Show game selection interface
  showGameSelection();

  Serial.println();
  Serial.println("=== How to play ===");
  Serial.println("1. Place all 32 pieces in the starting position.");
  Serial.println("2. A rainbow burst confirms the board, then 2 menu LEDs light up:");
  Serial.println("     D5 (left)  = Human vs Human  (white LED)");
  Serial.println("     E4 (right) = Human vs AI      (blue LED)");
  Serial.println("3. Lift any piece and place it on a lit square to choose.");
  Serial.println("   AI mode then asks for difficulty on 4 lit centre squares.");
  Serial.println("================================================");
}

// ---------------------------
// MAIN LOOP
// ---------------------------
void loop() {
  static unsigned long lastDebugPrint = 0;
  static bool firstLoop = true;
  
  if (firstLoop) {
    DBGLN("DEBUG: Entered main loop - waiting for game setup");
    firstLoop = false;
  }

  // Optional heartbeat - only when DEBUG_VERBOSE is enabled, so the serial
  // monitor stays quiet during normal play.
  if (DEBUG_VERBOSE && millis() - lastDebugPrint > 10000) {
    Serial.print("DEBUG: uptime ");
    Serial.print(millis() / 1000);
    Serial.println("s");
    lastDebugPrint = millis();
  }

#ifdef ENABLE_WIFI
  // Handle WiFi clients
  wifiManager.handleClient();
  
  // Check for WiFi game selection
  int selectedMode = wifiManager.getSelectedGameMode();
  if (selectedMode > 0) {
    Serial.print("DEBUG: WiFi game selection detected: ");
    Serial.println(selectedMode);
    
    switch (selectedMode) {
      case 1:
        currentMode = MODE_CHESS_MOVES;
        break;
      case 4:
        currentMode = MODE_SENSOR_TEST;
        break;
      default:
        Serial.println("Invalid game mode selected via WiFi");
        selectedMode = 0;
        break;
    }
    
    if (selectedMode > 0) {
      modeInitialized = false;
      boardDriver.clearAllLEDs();
      wifiManager.resetGameSelection();
      
      // Brief confirmation animation
      for (int i = 0; i < 3; i++) {
        boardDriver.setSquareLED(3, 3, 0, 255, 0, 0); // Green flash
        boardDriver.setSquareLED(3, 4, 0, 255, 0, 0);
        boardDriver.setSquareLED(4, 3, 0, 255, 0, 0);
        boardDriver.setSquareLED(4, 4, 0, 255, 0, 0);
        boardDriver.showLEDs();
        delay(200);
        boardDriver.clearAllLEDs();
        delay(200);
      }
    }
  }
#endif

  if (currentMode == MODE_SELECTION) {
    handleGameSelection();
  } else {
    static bool modeChangeLogged = false;
    if (!modeChangeLogged) {
      Serial.print("DEBUG: Mode changed to: ");
      Serial.println(currentMode);
      modeChangeLogged = true;
    }
    // Initialize the selected mode if not already done
    if (!modeInitialized) {
      initializeSelectedMode(currentMode);
      modeInitialized = true;
    }

    // ---------------------------------------------------------------------
    // Reset gesture: while in any game mode, if the user puts ALL 32 pieces
    // back onto their starting squares (and only those squares) and holds
    // them there for ~1.5 s, drop back to the menu. This lets the user end
    // a game and pick a different mode without power-cycling the board.
    //
    // To avoid triggering at the *start* of a game (where the starting
    // position is naturally satisfied), we require that the board has
    // first deviated from the starting position at least once since the
    // mode began. The flag resets whenever currentMode changes.
    // ---------------------------------------------------------------------
    if (currentMode == MODE_CHESS_MOVES || currentMode == MODE_CHESS_BOT) {
      static GameMode lastTrackedMode = MODE_SELECTION;
      static bool boardHasDeviated = false;
      static unsigned long resetHoldStart = 0;
      static bool wasInResetPos = false;

      if (currentMode != lastTrackedMode) {
        // New mode: reset the gesture state machine
        boardHasDeviated = false;
        wasInResetPos = false;
        resetHoldStart = 0;
        lastTrackedMode = currentMode;
      }

      boardDriver.readSensors();
      bool inResetPos = true;
      for (int r = 0; r < 8 && inResetPos; r++) {
        for (int c = 0; c < 8 && inResetPos; c++) {
          bool wantPiece = (STARTING_BOARD[r][c] != ' ');
          bool hasPiece = boardDriver.getSensorState(r, c);
          if (wantPiece != hasPiece) inResetPos = false;
        }
      }

      if (!inResetPos) {
        // Once the board ever deviates, future returns to the starting
        // position count as a real reset gesture.
        boardHasDeviated = true;
        wasInResetPos = false;
        resetHoldStart = 0;
      } else if (boardHasDeviated) {
        if (!wasInResetPos) {
          resetHoldStart = millis();
          wasInResetPos = true;
        } else if (millis() - resetHoldStart > 1500) {
          Serial.println("Reset gesture: 32 pieces back in starting position for >1.5s -> back to menu");
          persistenceClear(); // end the saved game; nothing to resume now
          currentMode = MODE_SELECTION;
          modeInitialized = false;
          modeChangeLogged = false;
          boardHasDeviated = false;
          wasInResetPos = false;
          resetHoldStart = 0;
          lastTrackedMode = MODE_SELECTION;
          boardDriver.clearAllLEDs();
          boardDriver.showLEDs();
          delay(50);
          return;
        }
      }
    }

    // Run the current game mode
    switch (currentMode) {
      case MODE_CHESS_MOVES:
        chessMoves.update();
        break;
      case MODE_CHESS_BOT:
        chessBot.update();
        break;
      case MODE_SENSOR_TEST:
        sensorTest.update();
        break;
      case MODE_GAME_3:
        // Future game modes - placeholder
        Serial.println("Game mode coming soon!");
        delay(1000);
        break;
      default:
        currentMode = MODE_SELECTION;
        modeInitialized = false;
        showGameSelection();
        break;
    }
  }
  
  delay(50); // Small delay to prevent overwhelming the system
}

// ---------------------------
// GAME SELECTION FUNCTIONS
// ---------------------------

// Standard chess starting position. Used during boot/menu so the user can
// already start placing pieces before picking a game mode -- the missing
// pieces are highlighted by BoardDriver::updateSetupDisplay (white side
// glows white, black side glows red, placed squares go dark).
const char STARTING_BOARD[8][8] = {
    {'R','N','B','Q','K','B','N','R'},
    {'P','P','P','P','P','P','P','P'},
    {' ',' ',' ',' ',' ',' ',' ',' '},
    {' ',' ',' ',' ',' ',' ',' ',' '},
    {' ',' ',' ',' ',' ',' ',' ',' '},
    {' ',' ',' ',' ',' ',' ',' ',' '},
    {'p','p','p','p','p','p','p','p'},
    {'r','n','b','q','k','b','n','r'}
};

void showGameSelection() {
  // Initial paint of the setup hint. Once the board is fully set up,
  // handleGameSelection() takes over and switches to the 2-selector menu
  // (D5 = AI, E4 = HvsH) after a convergent explosion animation.
  boardDriver.readSensors();
  boardDriver.updateSetupDisplay(STARTING_BOARD);
}

// Visual transition from "all pieces placed" -> "pick a mode". A
// multi-stage spectacle:
//   1. Four collapsing rainbow rings (red, orange, yellow, green) sweeping
//      from the outer edge to the centre.
//   2. Four diagonal beams shoot from each corner converging on the centre,
//      leaving a brief trail behind them.
//   3. A bright white shockwave bursts out from the centre and fades.
//   4. The 2 selector squares (D5, E4) pulse three times to catch the eye.
// Total ~3.5 seconds.
static void convergentExplosion() {
  const float cx = 3.5f;
  const float cy = 3.5f;

  // ---- Stage 1: 4 collapsing rainbow rings --------------------------------
  const uint8_t ringColors[4][3] = {
    {255,   0,   0},   // red
    {255, 100,   0},   // orange
    {255, 220,   0},   // yellow
    {  0, 220,  60},   // green
  };
  for (int wave = 0; wave < 4; wave++) {
    uint8_t r = ringColors[wave][0];
    uint8_t g = ringColors[wave][1];
    uint8_t b = ringColors[wave][2];
    for (float radius = 5.5f; radius >= -0.2f; radius -= 0.45f) {
      boardDriver.clearAllLEDs();
      for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
          float dx = (float)col - cx;
          float dy = (float)row - cy;
          float dist = sqrtf(dx * dx + dy * dy);
          float diff = fabsf(dist - radius);
          if (diff < 0.65f) {
            // Solid ring face
            boardDriver.setSquareLED(row, col, r, g, b);
          } else if (diff < 1.4f) {
            // Trailing edge - dim same hue for a softer look
            boardDriver.setSquareLED(row, col, r / 4, g / 4, b / 4);
          }
        }
      }
      boardDriver.showLEDs();
      delay(30);
    }
  }

  // ---- Stage 2: 4 diagonal beams from corners to the centre ---------------
  // Corners (0,0), (0,7), (7,0), (7,7) shoot inward simultaneously, advancing
  // 1 step per frame. Each beam leaves a 2-pixel fading trail.
  const int corners[4][2] = {{0,0},{0,7},{7,0},{7,7}};
  for (int step = 0; step < 4; step++) {
    boardDriver.clearAllLEDs();
    // Faint background ring at the current radius for depth
    float bgRadius = 4.0f - step;
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        float dx = (float)col - cx;
        float dy = (float)row - cy;
        float dist = sqrtf(dx * dx + dy * dy);
        if (fabsf(dist - bgRadius) < 0.5f) {
          boardDriver.setSquareLED(row, col, 30, 30, 80);
        }
      }
    }
    for (int c = 0; c < 4; c++) {
      int dr = (corners[c][0] == 0) ? 1 : -1;
      int dc = (corners[c][1] == 0) ? 1 : -1;
      int hr = corners[c][0] + dr * step;
      int hc = corners[c][1] + dc * step;
      // Trailing pixels
      for (int t = 0; t < 3; t++) {
        int tr = hr - dr * t;
        int tc = hc - dc * t;
        if (tr < 0 || tr > 7 || tc < 0 || tc > 7) continue;
        uint8_t bright = (t == 0) ? 255 : (t == 1) ? 120 : 40;
        // Each beam a different hue for variety
        switch (c) {
          case 0: boardDriver.setSquareLED(tr, tc, bright, 0, bright); break;            // magenta
          case 1: boardDriver.setSquareLED(tr, tc, 0, bright, bright); break;            // cyan
          case 2: boardDriver.setSquareLED(tr, tc, bright, bright / 2, 0); break;        // amber
          case 3: boardDriver.setSquareLED(tr, tc, bright / 2, bright, 0); break;        // lime
        }
      }
    }
    boardDriver.showLEDs();
    delay(70);
  }

  // ---- Stage 3: white shockwave outward from the centre -------------------
  for (float radius = 0.0f; radius < 6.5f; radius += 0.7f) {
    boardDriver.clearAllLEDs();
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        float dx = (float)col - cx;
        float dy = (float)row - cy;
        float dist = sqrtf(dx * dx + dy * dy);
        float diff = fabsf(dist - radius);
        if (diff < 0.7f) {
          boardDriver.setSquareLED(row, col, 0, 0, 0, 255); // pure white
        } else if (diff < 1.4f) {
          boardDriver.setSquareLED(row, col, 0, 0, 0, 80);
        }
      }
    }
    boardDriver.showLEDs();
    delay(45);
  }

  // ---- Stage 4: triple pulse on the 2 final selector squares --------------
  for (int flash = 0; flash < 3; flash++) {
    boardDriver.clearAllLEDs();
    boardDriver.setSquareLED(4, 3, 0, 0, 0, 255); // D5 = HvsH (white)
    boardDriver.setSquareLED(3, 4, 0, 0, 255, 0); // E4 = AI (blue)
    boardDriver.showLEDs();
    delay(180);
    boardDriver.clearAllLEDs();
    boardDriver.showLEDs();
    delay(120);
  }
}

void handleGameSelection() {
  static bool wasFullySetup = false;
  static bool menuArmed = false;  // true once the explosion has played and we're showing the 2-selector menu

  // checkInitialBoard refreshes sensors internally and returns true only when
  // every starting square has a piece on it. Extra pieces on empty squares
  // don't matter, which is what lets us reuse a single piece as the menu
  // selector below.
  bool isFullySetup = boardDriver.checkInitialBoard(STARTING_BOARD);

  // Phase 1: still placing pieces. Show the per-square hint (white/red).
  // Only happens before the menu is armed -- once the menu is up, lifting
  // a piece to use as a selector should not regress us back to setup.
  if (!menuArmed && !isFullySetup) {
    boardDriver.updateSetupDisplay(STARTING_BOARD);
    wasFullySetup = false;
    delay(50);
    return;
  }

  // Phase 2: just became fully set up -> play the convergent explosion once.
  if (!menuArmed && isFullySetup && !wasFullySetup) {
    Serial.println("Board fully set up - convergent explosion + menu reveal");
    convergentExplosion();
    wasFullySetup = true;
    menuArmed = true;
  }

  // Phase 3: 2-option menu (sticky once armed).
  //   D5 = (row 4, col 3) = Human vs Human
  //   E4 = (row 3, col 4) = AI mode
  // To pick a mode the user lifts any piece and places it on the lit
  // selector. The mode's own waitForBoardSetup() will then resume once
  // the board is back in its proper starting position.
  boardDriver.clearAllLEDs();
  boardDriver.setSquareLED(4, 3, 0, 0, 0, 255);  // D5 = HvsH (white)
  boardDriver.setSquareLED(3, 4, 0, 0, 255, 0);  // E4 = AI (blue)
  boardDriver.showLEDs();

  // Check for piece placement on the 2 selector squares
  if (boardDriver.getSensorState(4, 3)) {
    Serial.println("Chess Moves mode selected (Human vs Human)!");
    currentMode = MODE_CHESS_MOVES;
    modeInitialized = false;
    boardDriver.clearAllLEDs();
    // Reset menu state so a future return to selection re-runs the explosion
    wasFullySetup = false;
    menuArmed = false;
    delay(500); // debounce
  }
  else if (boardDriver.getSensorState(3, 4)) {
    Serial.println("Chess Bot mode selected (Human vs AI)!");
    currentMode = MODE_CHESS_BOT;
    modeInitialized = false;
    boardDriver.clearAllLEDs();
    wasFullySetup = false;
    menuArmed = false;
    delay(500);
  }

  delay(50);
}

// Ask the player to pick the AI strength before the bot connects to WiFi.
// Four empty centre squares (rank 4: c4 d4 e4 f4) light up as buttons; the
// player lifts any piece and places it on one. Returns the chosen difficulty.
// Mirrors the look & feel of the promotion picker in chess_moves.cpp.
BotDifficulty askBotDifficulty() {
  const int dr = 3;                  // internal row for rank 4 (always empty at setup)
  const int dcols[4] = {2, 3, 4, 5}; // files c, d, e, f
  // Per-button colour: Easy=green, Medium=blue, Hard=amber, Expert=red.
  const uint8_t cr[4] = {0,   0,   255, 255};
  const uint8_t cg[4] = {255, 0,   120, 0};
  const uint8_t cb[4] = {0,   255, 0,   0};

  boardDriver.clearAllLEDs();
  for (int i = 0; i < 4; i++) {
    boardDriver.setSquareLED(dr, dcols[i], cr[i], cg[i], cb[i]);
  }
  boardDriver.showLEDs();

  Serial.println("Choose AI difficulty - place a piece on a lit centre square:");
  Serial.println("  c4=Easy(green)  d4=Medium(blue)  e4=Hard(amber)  f4=Expert(red)");

  // Flush sensor state so only a NEW placement counts as a choice.
  boardDriver.readSensors();
  boardDriver.updateSensorPrev();

  BotDifficulty chosen = BOT_MEDIUM;
  int chosenIdx = 1;
  bool picked = false;
  while (!picked) {
    boardDriver.readSensors();
    for (int i = 0; i < 4; i++) {
      if (boardDriver.getSensorState(dr, dcols[i]) &&
          !boardDriver.getSensorPrev(dr, dcols[i])) {
        chosen = (BotDifficulty)(BOT_EASY + i);
        chosenIdx = i;
        picked = true;
        break;
      }
    }
    boardDriver.updateSensorPrev();
    delay(50);
  }

  // Confirm: blink the chosen square 3x in its own colour.
  for (int f = 0; f < 3; f++) {
    boardDriver.clearAllLEDs();
    boardDriver.setSquareLED(dr, dcols[chosenIdx], cr[chosenIdx], cg[chosenIdx], cb[chosenIdx]);
    boardDriver.showLEDs();
    delay(150);
    boardDriver.clearAllLEDs();
    boardDriver.showLEDs();
    delay(120);
  }

  // Wait for the selector piece to be lifted so it isn't mistaken for a move.
  while (true) {
    boardDriver.readSensors();
    bool anyOn = false;
    for (int i = 0; i < 4; i++) {
      if (boardDriver.getSensorState(dr, dcols[i])) { anyOn = true; break; }
    }
    if (!anyOn) break;
    delay(50);
  }
  boardDriver.clearAllLEDs();
  boardDriver.showLEDs();
  return chosen;
}

void initializeSelectedMode(GameMode mode) {
  switch (mode) {
    case MODE_CHESS_MOVES:
      Serial.println("Starting Chess Moves (Human vs Human)...");
      chessMoves.begin();
      break;
    case MODE_CHESS_BOT:
      Serial.println("Starting Chess Bot (Human vs AI)...");
      chessBot.setDifficulty(askBotDifficulty());
      chessBot.begin();
      break;
    case MODE_SENSOR_TEST:
      Serial.println("Starting Sensor Test...");
      sensorTest.begin();
      break;
    case MODE_GAME_3:
      Serial.println("This game mode will be available in a future update!");
      Serial.println("Returning to game selection in 3 seconds...");
      delay(3000);
      currentMode = MODE_SELECTION;
      modeInitialized = false;
      showGameSelection();
      break;
    default:
      currentMode = MODE_SELECTION;
      modeInitialized = false;
      showGameSelection();
      break;
  }
}
