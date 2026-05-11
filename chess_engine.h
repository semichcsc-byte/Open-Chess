#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <Arduino.h>

// Maximum number of moves a single piece can ever generate.
// 27 = queen on an empty board (8 ranks + 8 files + 11 diagonals
// minus the source square). Use 28 in arrays to leave a 1-slot guard
// and keep all callers in sync (was inconsistent: chess_moves used 27,
// chess_engine used 28).
#define MAX_MOVES_PER_PIECE 28

// Game outcome reported by getGameResult().
enum GameResult {
    GAME_ONGOING,
    GAME_CHECKMATE_WHITE_WINS,
    GAME_CHECKMATE_BLACK_WINS,
    GAME_STALEMATE,
    GAME_DRAW_50_MOVE,
    GAME_DRAW_INSUFFICIENT_MATERIAL
};

// Per-game state needed for rules that can't be derived from the board
// alone: castling rights, en-passant target, halfmove clock for the
// 50-move rule, and the last move (used by en-passant detection and the
// pretty-printer).
//
// The board representation in this firmware is:
//   row 0 = rank 1 (white back rank)
//   row 7 = rank 8 (black back rank)
// White pieces are uppercase ('R','N','B','Q','K','P'),
// black pieces are lowercase. ' ' means empty.
struct GameState {
    bool whiteCanCastleKingside;
    bool whiteCanCastleQueenside;
    bool blackCanCastleKingside;
    bool blackCanCastleQueenside;

    // En-passant target square. -1 / -1 if none.
    // This is the square *the capturing pawn moves to*, i.e. the square
    // behind the pawn that just made a 2-square advance.
    int enPassantRow;
    int enPassantCol;

    // Halfmove clock for the 50-move rule (incremented every ply,
    // reset on pawn move or capture). Triggers a draw at >= 100.
    int halfMoveClock;

    // Last move played, used by debug logs.
    int lastFromRow;
    int lastFromCol;
    int lastToRow;
    int lastToCol;
    char lastPiece;

    GameState() { reset(); }

    void reset() {
        whiteCanCastleKingside = true;
        whiteCanCastleQueenside = true;
        blackCanCastleKingside = true;
        blackCanCastleQueenside = true;
        enPassantRow = -1;
        enPassantCol = -1;
        halfMoveClock = 0;
        lastFromRow = lastFromCol = -1;
        lastToRow = lastToCol = -1;
        lastPiece = ' ';
    }
};

// ---------------------------
// Chess Engine Class
// ---------------------------
class ChessEngine {
private:
    // Helper functions for pseudo-legal move generation (do not filter
    // moves that leave own king in check; use getLegalMoves for that).
    void addPawnMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]);
    void addPawnMovesEP(const char board[8][8], const GameState* state, int row, int col, char pieceColor, int &moveCount, int moves[][2]);
    void addRookMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]);
    void addKnightMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]);
    void addBishopMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]);
    void addQueenMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]);
    void addKingMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]);
    void addCastlingMoves(const char board[8][8], const GameState* state, int row, int col, char pieceColor, int &moveCount, int moves[][2]);

    bool isSquareOccupiedByOpponent(const char board[8][8], int row, int col, char pieceColor);
    bool isSquareEmpty(const char board[8][8], int row, int col);
    bool isValidSquare(int row, int col);

    // Find the king of the given color. Returns false if not found.
    bool findKing(const char board[8][8], char color, int &kingRow, int &kingCol);

public:
    ChessEngine();

    // Pseudo-legal moves (does not filter own-check). Pass state=nullptr
    // for tests / contexts where castling and en-passant aren't needed.
    void getPossibleMoves(const char board[8][8], int row, int col, int &moveCount, int moves[][2]);

    // Legal moves: pseudo-legal filtered by own-check, and including
    // castling + en-passant when state is provided.
    void getLegalMoves(const char board[8][8], const GameState* state, int row, int col, int &moveCount, int moves[][2]);

    // Move validation
    bool isValidMove(const char board[8][8], int fromRow, int fromCol, int toRow, int toCol);
    bool isLegalMove(const char board[8][8], const GameState* state, int fromRow, int fromCol, int toRow, int toCol);

    // Attack / check detection
    bool isSquareAttacked(const char board[8][8], int row, int col, char byColor);
    bool isInCheck(const char board[8][8], char color);

    // Outcome detection
    bool hasAnyLegalMove(const char board[8][8], const GameState* state, char color);
    GameResult getGameResult(const char board[8][8], const GameState* state, char colorToMove);

    // Apply a move to the board AND update the state. Handles regular
    // moves, captures, castling (moves the rook too), en-passant
    // (removes the captured pawn from its actual square), and updates
    // castling rights / en-passant target / halfmove clock. Promotion
    // is NOT done here; pass the promoted piece via promoteTo (or 0 to
    // skip).
    void applyMove(char board[8][8], GameState* state,
                   int fromRow, int fromCol, int toRow, int toCol,
                   char promoteTo = 0);

    // Game state checks
    bool isPawnPromotion(char piece, int targetRow);
    char getPromotedPiece(char piece);
    bool isCastlingMove(const char board[8][8], int fromRow, int fromCol, int toRow, int toCol);
    bool isEnPassantMove(const char board[8][8], const GameState* state, int fromRow, int fromCol, int toRow, int toCol);

    // Utility
    char getPieceColor(char piece); // public so callers can ask
    void printMove(int fromRow, int fromCol, int toRow, int toCol);
    char algebraicToCol(char file);
    int algebraicToRow(int rank);

    // Self-tests run at boot. Returns number of failures (0 = all pass).
    // Prints results to Serial.
    int runSelfTests();
};

#endif // CHESS_ENGINE_H
