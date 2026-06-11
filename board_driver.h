#ifndef BOARD_DRIVER_H
#define BOARD_DRIVER_H

#include <Adafruit_NeoPixel.h>

// ---------------------------
// Hardware Configuration
// ---------------------------
#define LED_PIN     17       // Pin for NeoPixels
#define NUM_ROWS    8
#define NUM_COLS    8
#define LED_COUNT   (NUM_ROWS * NUM_COLS)
#define BRIGHTNESS  100

// Debounce: a sensor reading must be the same for this many consecutive
// scans before the public state flips. Each readSensors() call adds 1
// scan, so with the ~50ms loop delay this is roughly N*50ms of stability.
// Tune up if pieces give false-positive flickers when sliding.
#define SENSOR_DEBOUNCE_SCANS 3

// Shift Register (74HC594) Pins
#define SER_PIN     2   // Serial data input (74HC594 pin 14)
#define SRCLK_PIN   3   // Shift register clock (pin 11)
#define RCLK_PIN    4   // Latch clock (pin 12)

// Column Input Pins (D6..D13)
#define COL_PINS {6, 7, 8, 9, 10, 11, 12, 13}

// ---------------------------
// Board Driver Class
// ---------------------------
class BoardDriver {
private:
    Adafruit_NeoPixel strip;
    int colPins[NUM_COLS];
    byte rowPatterns[8];
    bool sensorState[8][8];
    bool sensorPrev[8][8];

    // Debounce buffers: latest raw read, count of consecutive identical
    // raw reads since the public state last changed.
    bool sensorRawLast[8][8];
    uint8_t sensorStableCount[8][8];

    void loadShiftRegister(byte data);
    int getPixelIndex(int row, int col);

public:
    BoardDriver();
    void begin();
    void readSensors();
    bool getSensorState(int row, int col);
    bool getSensorPrev(int row, int col);
    void updateSensorPrev();
    
    // LED Control
    void clearAllLEDs();
    void setSquareLED(int row, int col, uint32_t color);
    void setSquareLED(int row, int col, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);
    void showLEDs();
    
    // Animation Functions
    void fireworkAnimation();
    void captureAnimation();
    void promotionAnimation(int col);
    void blinkSquare(int row, int col, int times = 3);
    void highlightSquare(int row, int col, uint32_t color);

    // Non-blocking "whose turn" indicator: gently breathes one full row
    // (a player's back rank) in the given colour. Call it every loop while
    // idle; it only repaints when the brightness step changes, so it is
    // cheap and flicker-free. Pass the row to light (0 = rank 1, 7 = rank 8).
    void breatheRow(int row, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);
    
    // Setup Functions
    bool checkInitialBoard(const char initialBoard[8][8]);
    void updateSetupDisplay(const char initialBoard[8][8]);
    void printBoardState(const char initialBoard[8][8]);
};

#endif // BOARD_DRIVER_H
