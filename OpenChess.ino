#include "board_driver.h"
#include "chess_engine.h"
#include "chess_moves.h"
#include "sensor_test.h"
#include "chess_bot.h"

#include <math.h>

// Forward declaration: defined later in this file (next to the other
// game-selection helpers). Needed because the reset-gesture code in loop()
// references it before its definition.
extern const char STARTING_BOARD[8][8];

// Firmware version printed at boot for support / debugging.
#define OPENCHESS_FW_VERSION "1.2.0-rp2040"

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
  Serial.println("DEBUG: Serial communication established");
  Serial.print("DEBUG: Millis since boot: ");
  Serial.println(millis());
  
  // Debug board type detection
  Serial.println("DEBUG: Board type detection:");
  #if defined(ARDUINO_SAMD_MKRWIFI1010)
  Serial.println("  - Detected: ARDUINO_SAMD_MKRWIFI1010");
  #elif defined(ARDUINO_SAMD_NANO_33_IOT)
  Serial.println("  - Detected: ARDUINO_SAMD_NANO_33_IOT");
  #elif defined(ARDUINO_NANO_RP2040_CONNECT)
  Serial.println("  - Detected: ARDUINO_NANO_RP2040_CONNECT");
  #else
  Serial.println("  - Detected: Unknown/Other board type");
  #endif
  
  // Check which mode is compiled
#ifdef ENABLE_WIFI
  Serial.println("DEBUG: Compiled with ENABLE_WIFI defined");
#else
  Serial.println("DEBUG: Compiled without ENABLE_WIFI (local mode only)");
#endif

  Serial.println("DEBUG: About to initialize board driver...");
  // Initialize board driver
  boardDriver.begin();
  Serial.println("DEBUG: Board driver initialized successfully");

#ifdef ENABLE_WIFI
  Serial.println();
  Serial.println("=== WiFi Mode Enabled ===");
  Serial.println("DEBUG: About to initialize WiFi Manager...");
  Serial.println("DEBUG: This will attempt to create Access Point");
  
  // Initialize WiFi Manager
  wifiManager.begin();
  
  Serial.println("DEBUG: WiFi Manager initialization completed");
  Serial.println("If WiFi AP was created successfully, you should see:");
  Serial.println("- Network name: OpenChessBoard");
  Serial.println("- Password: chess123");
  Serial.println("- Web interface: http://192.168.4.1");
  Serial.println("Or place a piece on the board for local selection");
#else
  Serial.println();
  Serial.println("=== Local Mode Only ===");
  Serial.println("WiFi features are disabled in this build");
  Serial.println("To enable WiFi: Uncomment #define ENABLE_WIFI and recompile");
#endif

  Serial.println();
  Serial.println("=== Game Selection Mode ===");
  Serial.println("DEBUG: About to show game selection LEDs...");

  // Show game selection interface
  showGameSelection();
  
  Serial.println("DEBUG: Game selection LEDs should now be visible");
  Serial.println("Four white LEDs should be lit in the center of the board:");
  Serial.println("Position 1 (3,3): Chess Moves (Human vs Human)");
  Serial.println("Position 2 (3,4): Chess Bot (Human vs AI)");
  Serial.println("Position 3 (4,3): Game Mode 3 (Coming Soon)");
  Serial.println("Position 4 (4,4): Sensor Test");
  Serial.println();
  Serial.println("Place any chess piece on a white LED to select that mode");
  Serial.println("================================================");
  Serial.println("         Setup Complete - Entering Main Loop");
  Serial.println("================================================");
}

// ---------------------------
// MAIN LOOP
// ---------------------------
void loop() {
  static unsigned long lastDebugPrint = 0;
  static bool firstLoop = true;
  
  if (firstLoop) {
    Serial.println("DEBUG: Entered main loop - system is running");
    firstLoop = false;
  }
  
  // Print periodic status every 10 seconds
  if (millis() - lastDebugPrint > 10000) {
    Serial.print("DEBUG: Loop running, uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
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
    boardDriver.setSquareLED(4, 3, 0, 0, 0, 255); // D5 = HvsH
    boardDriver.setSquareLED(3, 4, 0, 0, 0, 255); // E4 = AI
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
  boardDriver.setSquareLED(4, 3, 0, 0, 0, 255);  // D5 = HvsH
  boardDriver.setSquareLED(3, 4, 0, 0, 0, 255);  // E4 = AI
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

void initializeSelectedMode(GameMode mode) {
  switch (mode) {
    case MODE_CHESS_MOVES:
      Serial.println("Starting Chess Moves (Human vs Human)...");
      chessMoves.begin();
      break;
    case MODE_CHESS_BOT:
      Serial.println("Starting Chess Bot (Human vs AI)...");
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
