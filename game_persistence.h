#ifndef GAME_PERSISTENCE_H
#define GAME_PERSISTENCE_H

#include <Arduino.h>
#include "chess_engine.h"  // GameState

// A snapshot of an in-progress game, persisted to a reserved flash region so
// the board can silently resume after a power cycle. The game stays "active"
// until the player puts all 32 pieces back on the starting squares (the reset
// gesture), which clears it. ~80 bytes.
struct SavedGame {
    uint32_t magic;        // sentinel, must equal SAVED_GAME_MAGIC
    uint8_t  version;      // schema version
    uint8_t  active;       // 1 = a game is in progress
    uint8_t  mode;         // 1 = Human-vs-Human, 2 = Human-vs-AI
    uint8_t  difficulty;   // BotDifficulty (AI mode only)
    char     board[8][8];  // piece chars, ' ' = empty
    char     turnColor;    // 'w' or 'b' (side to move)
    // Flattened GameState:
    uint8_t  castling;     // bit0 WK, bit1 WQ, bit2 BK, bit3 BQ
    int8_t   epRow;
    int8_t   epCol;
    int16_t  halfMoveClock;
    int8_t   lastFromRow;
    int8_t   lastFromCol;
    int8_t   lastToRow;
    int8_t   lastToCol;
    char     lastPiece;
};

// Initialise the flash-backed store. Returns false if flash is unavailable;
// in that case the game still runs, it just won't survive a power cycle.
bool persistenceBegin();

// Save the current game. mode: 1=HvH, 2=AI. Safe to call after every move.
void persistenceSaveGame(uint8_t mode, uint8_t difficulty, char turnColor,
                         const char board[8][8], const GameState& state);

// Load the saved game. Returns true only if a valid, active game exists.
bool persistenceLoadGame(SavedGame& out);

// Forget any saved game (called on the reset gesture / new game).
void persistenceClear();

// Map a SavedGame's flattened fields back into a GameState.
void persistenceUnpackState(const SavedGame& g, GameState& state);

#endif // GAME_PERSISTENCE_H
