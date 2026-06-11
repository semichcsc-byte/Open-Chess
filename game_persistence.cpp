#include "game_persistence.h"
#include "stockfish_settings.h"  // BotDifficulty

#include "FlashIAP.h"
#include "FlashIAPBlockDevice.h"
#include "TDBStore.h"

using namespace mbed;

#define SAVED_GAME_MAGIC   0x0C4E5301UL
#define SAVED_GAME_VERSION 1
#define SAVED_GAME_KEY     "game"

// Reserve the top 64 KB of internal flash for the key-value store. The sketch
// is ~150 KB and lives at the bottom of the 2 MB flash, so this region is free
// and is not touched by an `arduino-cli upload` of the program.
#define PERSIST_REGION_SIZE (64 * 1024)

static FlashIAPBlockDevice* s_bd = nullptr;
static TDBStore*            s_store = nullptr;
static bool                 s_ready = false;

bool persistenceBegin() {
    if (s_ready) return true;

    // Discover flash geometry so we can place the store at the very top,
    // away from the program region.
    FlashIAP flash;
    if (flash.init() != 0) return false;
    uint32_t flashStart = flash.get_flash_start();
    uint32_t flashSize  = flash.get_flash_size();
    flash.deinit();

    uint32_t baseAddr = flashStart + flashSize - PERSIST_REGION_SIZE;

    static FlashIAPBlockDevice bd(baseAddr, PERSIST_REGION_SIZE);
    if (bd.init() != 0) return false;
    s_bd = &bd;

    static TDBStore store(&bd);
    if (store.init() != MBED_SUCCESS) return false;
    s_store = &store;

    s_ready = true;
    return true;
}

void persistenceSaveGame(uint8_t mode, uint8_t difficulty, char turnColor,
                         const char board[8][8], const GameState& state) {
    if (!s_ready) return;

    SavedGame g;
    memset(&g, 0, sizeof(g));
    g.magic      = SAVED_GAME_MAGIC;
    g.version    = SAVED_GAME_VERSION;
    g.active     = 1;
    g.mode       = mode;
    g.difficulty = difficulty;
    g.turnColor  = turnColor;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            g.board[r][c] = board[r][c];

    g.castling = 0;
    if (state.whiteCanCastleKingside)  g.castling |= 0x1;
    if (state.whiteCanCastleQueenside) g.castling |= 0x2;
    if (state.blackCanCastleKingside)  g.castling |= 0x4;
    if (state.blackCanCastleQueenside) g.castling |= 0x8;
    g.epRow         = (int8_t)state.enPassantRow;
    g.epCol         = (int8_t)state.enPassantCol;
    g.halfMoveClock = (int16_t)state.halfMoveClock;
    g.lastFromRow   = (int8_t)state.lastFromRow;
    g.lastFromCol   = (int8_t)state.lastFromCol;
    g.lastToRow     = (int8_t)state.lastToRow;
    g.lastToCol     = (int8_t)state.lastToCol;
    g.lastPiece     = state.lastPiece;

    s_store->set(SAVED_GAME_KEY, &g, sizeof(g), 0);
}

bool persistenceLoadGame(SavedGame& out) {
    if (!s_ready) return false;

    size_t actual = 0;
    int rc = s_store->get(SAVED_GAME_KEY, &out, sizeof(out), &actual);
    if (rc != MBED_SUCCESS || actual != sizeof(out)) return false;
    if (out.magic != SAVED_GAME_MAGIC) return false;
    if (out.version != SAVED_GAME_VERSION) return false;
    if (!out.active) return false;
    return true;
}

void persistenceClear() {
    if (!s_ready) return;
    s_store->remove(SAVED_GAME_KEY);
}

void persistenceUnpackState(const SavedGame& g, GameState& state) {
    state.whiteCanCastleKingside  = (g.castling & 0x1) != 0;
    state.whiteCanCastleQueenside = (g.castling & 0x2) != 0;
    state.blackCanCastleKingside  = (g.castling & 0x4) != 0;
    state.blackCanCastleQueenside = (g.castling & 0x8) != 0;
    state.enPassantRow = g.epRow;
    state.enPassantCol = g.epCol;
    state.halfMoveClock = g.halfMoveClock;
    state.lastFromRow = g.lastFromRow;
    state.lastFromCol = g.lastFromCol;
    state.lastToRow   = g.lastToRow;
    state.lastToCol   = g.lastToCol;
    state.lastPiece   = g.lastPiece;
}
