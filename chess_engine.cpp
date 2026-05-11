#include "chess_engine.h"
#include <Arduino.h>

// ---------------------------
// ChessEngine Implementation
// ---------------------------

ChessEngine::ChessEngine() {
    // Constructor - nothing to initialize for now
}

// Main move generation function
void ChessEngine::getPossibleMoves(const char board[8][8], int row, int col, int &moveCount, int moves[][2]) {
    moveCount = 0;
    char piece = board[row][col];
    
    if (piece == ' ') return; // Empty square
    
    char pieceColor = getPieceColor(piece);
    
    // Convert to uppercase for easier comparison
    piece = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;

    switch(piece) {
        case 'P': // Pawn
            addPawnMoves(board, row, col, pieceColor, moveCount, moves);
            break;
        case 'R': // Rook
            addRookMoves(board, row, col, pieceColor, moveCount, moves);
            break;
        case 'N': // Knight
            addKnightMoves(board, row, col, pieceColor, moveCount, moves);
            break;
        case 'B': // Bishop
            addBishopMoves(board, row, col, pieceColor, moveCount, moves);
            break;
        case 'Q': // Queen
            addQueenMoves(board, row, col, pieceColor, moveCount, moves);
            break;
        case 'K': // King
            addKingMoves(board, row, col, pieceColor, moveCount, moves);
            break;
    }
}

// Pawn move generation
void ChessEngine::addPawnMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]) {
    int direction = (pieceColor == 'w') ? 1 : -1;
    
    // One square forward
    if (isValidSquare(row + direction, col) && isSquareEmpty(board, row + direction, col)) {
        moves[moveCount][0] = row + direction;
        moves[moveCount][1] = col;
        moveCount++;
        
        // Initial two-square move
        if ((pieceColor == 'w' && row == 1) || (pieceColor == 'b' && row == 6)) {
            if (isSquareEmpty(board, row + 2*direction, col)) {
                moves[moveCount][0] = row + 2*direction;
                moves[moveCount][1] = col;
                moveCount++;
            }
        }
    }
    
    // Diagonal captures
    int captureColumns[] = {col-1, col+1};
    for (int i = 0; i < 2; i++) {
        int captureRow = row + direction;
        int captureCol = captureColumns[i];
        
        if (isValidSquare(captureRow, captureCol) && 
            isSquareOccupiedByOpponent(board, captureRow, captureCol, pieceColor)) {
            moves[moveCount][0] = captureRow;
            moves[moveCount][1] = captureCol;
            moveCount++;
        }
    }
}

// Rook move generation
void ChessEngine::addRookMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]) {
    int directions[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    
    for (int d = 0; d < 4; d++) {
        for (int step = 1; step < 8; step++) {
            int newRow = row + step * directions[d][0];
            int newCol = col + step * directions[d][1];
            
            if (!isValidSquare(newRow, newCol)) break;
            
            if (isSquareEmpty(board, newRow, newCol)) {
                moves[moveCount][0] = newRow;
                moves[moveCount][1] = newCol;
                moveCount++;
            } else {
                // Check if it's a capturable piece
                if (isSquareOccupiedByOpponent(board, newRow, newCol, pieceColor)) {
                    moves[moveCount][0] = newRow;
                    moves[moveCount][1] = newCol;
                    moveCount++;
                }
                break; // Can't move past any piece
            }
        }
    }
}

// Knight move generation
void ChessEngine::addKnightMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]) {
    int knightMoves[8][2] = {{2,1}, {1,2}, {-1,2}, {-2,1},
                             {-2,-1}, {-1,-2}, {1,-2}, {2,-1}};
    
    for (int i = 0; i < 8; i++) {
        int newRow = row + knightMoves[i][0];
        int newCol = col + knightMoves[i][1];
        
        if (isValidSquare(newRow, newCol)) {
            if (isSquareEmpty(board, newRow, newCol) || 
                isSquareOccupiedByOpponent(board, newRow, newCol, pieceColor)) {
                moves[moveCount][0] = newRow;
                moves[moveCount][1] = newCol;
                moveCount++;
            }
        }
    }
}

// Bishop move generation
void ChessEngine::addBishopMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]) {
    int directions[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    
    for (int d = 0; d < 4; d++) {
        for (int step = 1; step < 8; step++) {
            int newRow = row + step * directions[d][0];
            int newCol = col + step * directions[d][1];
            
            if (!isValidSquare(newRow, newCol)) break;
            
            if (isSquareEmpty(board, newRow, newCol)) {
                moves[moveCount][0] = newRow;
                moves[moveCount][1] = newCol;
                moveCount++;
            } else {
                // Check if it's a capturable piece
                if (isSquareOccupiedByOpponent(board, newRow, newCol, pieceColor)) {
                    moves[moveCount][0] = newRow;
                    moves[moveCount][1] = newCol;
                    moveCount++;
                }
                break; // Can't move past any piece
            }
        }
    }
}

// Queen move generation (combination of rook and bishop)
void ChessEngine::addQueenMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]) {
    addRookMoves(board, row, col, pieceColor, moveCount, moves);
    addBishopMoves(board, row, col, pieceColor, moveCount, moves);
}

// King move generation
void ChessEngine::addKingMoves(const char board[8][8], int row, int col, char pieceColor, int &moveCount, int moves[][2]) {
    int kingMoves[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1},
                           {1,1}, {1,-1}, {-1,1}, {-1,-1}};
    
    for (int i = 0; i < 8; i++) {
        int newRow = row + kingMoves[i][0];
        int newCol = col + kingMoves[i][1];
        
        if (isValidSquare(newRow, newCol)) {
            if (isSquareEmpty(board, newRow, newCol) || 
                isSquareOccupiedByOpponent(board, newRow, newCol, pieceColor)) {
                moves[moveCount][0] = newRow;
                moves[moveCount][1] = newCol;
                moveCount++;
            }
        }
    }
}

// Helper function to check if a square is occupied by an opponent piece
bool ChessEngine::isSquareOccupiedByOpponent(const char board[8][8], int row, int col, char pieceColor) {
    char targetPiece = board[row][col];
    if (targetPiece == ' ') return false;
    
    char targetColor = getPieceColor(targetPiece);
    return targetColor != pieceColor;
}

// Helper function to check if a square is empty
bool ChessEngine::isSquareEmpty(const char board[8][8], int row, int col) {
    return board[row][col] == ' ';
}

// Helper function to check if coordinates are within board bounds
bool ChessEngine::isValidSquare(int row, int col) {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

// Helper function to get piece color
char ChessEngine::getPieceColor(char piece) {
    return (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
}

// Move validation
bool ChessEngine::isValidMove(const char board[8][8], int fromRow, int fromCol, int toRow, int toCol) {
    int moveCount = 0;
    int moves[MAX_MOVES_PER_PIECE][2]; // sized via chess_engine.h constant
    
    getPossibleMoves(board, fromRow, fromCol, moveCount, moves);
    
    for (int i = 0; i < moveCount; i++) {
        if (moves[i][0] == toRow && moves[i][1] == toCol) {
            return true;
        }
    }
    return false;
}

// Check if a pawn move results in promotion
bool ChessEngine::isPawnPromotion(char piece, int targetRow) {
    if (piece == 'P' && targetRow == 7) return true;  // White pawn reaches 8th rank
    if (piece == 'p' && targetRow == 0) return true;  // Black pawn reaches 1st rank
    return false;
}

// Get the promoted piece (always queen for now)
char ChessEngine::getPromotedPiece(char piece) {
    return (piece == 'P') ? 'Q' : 'q';
}

// Utility function to print a move in readable format
void ChessEngine::printMove(int fromRow, int fromCol, int toRow, int toCol) {
    Serial.print((char)('a' + fromCol));
    Serial.print(fromRow + 1);
    Serial.print(" to ");
    Serial.print((char)('a' + toCol));
    Serial.println(toRow + 1);
}

// Convert algebraic notation file (a-h) to column index (0-7)
char ChessEngine::algebraicToCol(char file) {
    return file - 'a';
}

// Convert algebraic notation rank (1-8) to row index (0-7)
int ChessEngine::algebraicToRow(int rank) {
    return rank - 1;
}

// =============================================================
// Phase 2: legal-move filtering, check/checkmate, castling,
// en passant, draws, and self-tests.
// =============================================================

bool ChessEngine::findKing(const char board[8][8], char color, int &kingRow, int &kingCol) {
    char target = (color == 'w') ? 'K' : 'k';
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == target) {
                kingRow = r;
                kingCol = c;
                return true;
            }
        }
    }
    return false;
}

bool ChessEngine::isSquareAttacked(const char board[8][8], int row, int col, char byColor) {
    // Iterate every opponent piece and check if it pseudo-attacks (row,col).
    // For pawns we hand-roll the diagonal capture pattern instead of going
    // through addPawnMoves (which would also include the forward push and
    // a 2-square push that aren't attacks).
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char piece = board[r][c];
            if (piece == ' ') continue;
            if (getPieceColor(piece) != byColor) continue;

            char up = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;

            if (up == 'P') {
                int dir = (byColor == 'w') ? 1 : -1;
                if (r + dir == row && (c - 1 == col || c + 1 == col)) return true;
                continue;
            }

            // For non-pawn pieces, generate pseudo-legal moves and check
            // if any lands on (row,col). We use the existing helpers here;
            // they return both quiet moves and captures, which is fine
            // because we just want to know whether the square is reachable.
            int moveCount = 0;
            int moves[MAX_MOVES_PER_PIECE][2];
            switch (up) {
                case 'R': addRookMoves(board, r, c, byColor, moveCount, moves); break;
                case 'N': addKnightMoves(board, r, c, byColor, moveCount, moves); break;
                case 'B': addBishopMoves(board, r, c, byColor, moveCount, moves); break;
                case 'Q': addQueenMoves(board, r, c, byColor, moveCount, moves); break;
                case 'K': {
                    // For the king, only adjacent squares attack — we cannot
                    // call addKingMoves recursively from inside isInCheck via
                    // addCastlingMoves, but plain king moves are safe.
                    int kingDeltas[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
                    for (int i = 0; i < 8; i++) {
                        if (r + kingDeltas[i][0] == row && c + kingDeltas[i][1] == col) return true;
                    }
                    continue;
                }
                default: continue;
            }
            for (int i = 0; i < moveCount; i++) {
                if (moves[i][0] == row && moves[i][1] == col) return true;
            }
        }
    }
    return false;
}

bool ChessEngine::isInCheck(const char board[8][8], char color) {
    int kr, kc;
    if (!findKing(board, color, kr, kc)) return false;
    char attacker = (color == 'w') ? 'b' : 'w';
    return isSquareAttacked(board, kr, kc, attacker);
}

void ChessEngine::addPawnMovesEP(const char board[8][8], const GameState* state, int row, int col, char pieceColor, int &moveCount, int moves[][2]) {
    if (!state || state->enPassantRow < 0) return;
    int dir = (pieceColor == 'w') ? 1 : -1;
    // EP target is the square *behind* the just-advanced enemy pawn,
    // so a capturing pawn must move row+dir == enPassantRow and
    // col +/- 1 == enPassantCol.
    if (row + dir == state->enPassantRow && (col - 1 == state->enPassantCol || col + 1 == state->enPassantCol)) {
        moves[moveCount][0] = state->enPassantRow;
        moves[moveCount][1] = state->enPassantCol;
        moveCount++;
    }
}

void ChessEngine::addCastlingMoves(const char board[8][8], const GameState* state, int row, int col, char pieceColor, int &moveCount, int moves[][2]) {
    if (!state) return;

    // The king must be on its home square and not in check.
    if (pieceColor == 'w' && (row != 0 || col != 4)) return;
    if (pieceColor == 'b' && (row != 7 || col != 4)) return;
    char enemy = (pieceColor == 'w') ? 'b' : 'w';
    if (isSquareAttacked(board, row, col, enemy)) return;

    bool canKingside  = (pieceColor == 'w') ? state->whiteCanCastleKingside  : state->blackCanCastleKingside;
    bool canQueenside = (pieceColor == 'w') ? state->whiteCanCastleQueenside : state->blackCanCastleQueenside;
    char rookChar = (pieceColor == 'w') ? 'R' : 'r';

    if (canKingside) {
        // Squares between king (col 4) and kingside rook (col 7) must be empty,
        // and the king's path (cols 4,5,6) must not be attacked.
        if (board[row][5] == ' ' && board[row][6] == ' ' && board[row][7] == rookChar
            && !isSquareAttacked(board, row, 5, enemy)
            && !isSquareAttacked(board, row, 6, enemy)) {
            moves[moveCount][0] = row;
            moves[moveCount][1] = 6;
            moveCount++;
        }
    }
    if (canQueenside) {
        // Squares b/c/d (cols 1,2,3) must be empty between king (col 4) and rook (col 0).
        // King's path (cols 4,3,2) must not be attacked. b-file (col 1) needs empty
        // but not unattacked (rook can pass through attack).
        if (board[row][1] == ' ' && board[row][2] == ' ' && board[row][3] == ' ' && board[row][0] == rookChar
            && !isSquareAttacked(board, row, 3, enemy)
            && !isSquareAttacked(board, row, 2, enemy)) {
            moves[moveCount][0] = row;
            moves[moveCount][1] = 2;
            moveCount++;
        }
    }
}

bool ChessEngine::isCastlingMove(const char board[8][8], int fromRow, int fromCol, int toRow, int toCol) {
    char piece = board[fromRow][fromCol];
    if (piece != 'K' && piece != 'k') return false;
    if (fromRow != toRow) return false;
    int delta = toCol - fromCol;
    return (delta == 2 || delta == -2);
}

bool ChessEngine::isEnPassantMove(const char board[8][8], const GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    if (!state || state->enPassantRow < 0) return false;
    char piece = board[fromRow][fromCol];
    if (piece != 'P' && piece != 'p') return false;
    if (toRow != state->enPassantRow || toCol != state->enPassantCol) return false;
    // Diagonal pawn move to an empty square => EP.
    return (board[toRow][toCol] == ' ' && fromCol != toCol);
}

void ChessEngine::getLegalMoves(const char board[8][8], const GameState* state, int row, int col, int &moveCount, int moves[][2]) {
    // 1) start with pseudo-legal moves
    int pseudoCount = 0;
    int pseudo[MAX_MOVES_PER_PIECE][2];
    getPossibleMoves(board, row, col, pseudoCount, pseudo);

    char piece = board[row][col];
    if (piece == ' ') { moveCount = 0; return; }
    char pieceColor = getPieceColor(piece);

    // 2) augment with castling + en-passant when state available
    if (state) {
        char up = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;
        if (up == 'K') addCastlingMoves(board, state, row, col, pieceColor, pseudoCount, pseudo);
        if (up == 'P') addPawnMovesEP(board, state, row, col, pieceColor, pseudoCount, pseudo);
    }

    // 3) filter out moves that would leave own king in check
    moveCount = 0;
    for (int i = 0; i < pseudoCount; i++) {
        int tr = pseudo[i][0];
        int tc = pseudo[i][1];

        // Simulate the move on a scratch board
        char scratch[8][8];
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++) scratch[r][c] = board[r][c];

        char captured = scratch[tr][tc];
        scratch[tr][tc] = piece;
        scratch[row][col] = ' ';

        // Castling: also move the rook on the scratch board so the
        // king-attacked check after the move is realistic (a rook
        // pinned through the king matters).
        if (state) {
            char up = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;
            if (up == 'K' && (tc - col == 2 || tc - col == -2)) {
                if (tc == 6) { // kingside
                    scratch[tr][5] = scratch[tr][7];
                    scratch[tr][7] = ' ';
                } else if (tc == 2) { // queenside
                    scratch[tr][3] = scratch[tr][0];
                    scratch[tr][0] = ' ';
                }
            }
            // En passant: remove the captured pawn from its actual square
            if (up == 'P' && tc != col && captured == ' ' && state->enPassantRow == tr && state->enPassantCol == tc) {
                int capturedPawnRow = (pieceColor == 'w') ? tr - 1 : tr + 1;
                scratch[capturedPawnRow][tc] = ' ';
            }
        }

        if (!isInCheck(scratch, pieceColor)) {
            moves[moveCount][0] = tr;
            moves[moveCount][1] = tc;
            moveCount++;
        }
    }
}

bool ChessEngine::isLegalMove(const char board[8][8], const GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    int moveCount = 0;
    int moves[MAX_MOVES_PER_PIECE][2];
    getLegalMoves(board, state, fromRow, fromCol, moveCount, moves);
    for (int i = 0; i < moveCount; i++) {
        if (moves[i][0] == toRow && moves[i][1] == toCol) return true;
    }
    return false;
}

bool ChessEngine::hasAnyLegalMove(const char board[8][8], const GameState* state, char color) {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char piece = board[r][c];
            if (piece == ' ') continue;
            if (getPieceColor(piece) != color) continue;
            int moveCount = 0;
            int moves[MAX_MOVES_PER_PIECE][2];
            getLegalMoves(board, state, r, c, moveCount, moves);
            if (moveCount > 0) return true;
        }
    }
    return false;
}

GameResult ChessEngine::getGameResult(const char board[8][8], const GameState* state, char colorToMove) {
    // Insufficient material: K vs K, K+B vs K, K+N vs K, K+B vs K+B (same color squares not checked here)
    int wPieces = 0, bPieces = 0;
    int wMinor = 0, bMinor = 0;
    bool heavyOrPawn = false;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char p = board[r][c];
            if (p == ' ') continue;
            char up = (p >= 'a' && p <= 'z') ? p - 32 : p;
            bool isWhite = (p == up);
            if (up == 'P' || up == 'R' || up == 'Q') heavyOrPawn = true;
            if (isWhite) {
                wPieces++;
                if (up == 'B' || up == 'N') wMinor++;
            } else {
                bPieces++;
                if (up == 'B' || up == 'N') bMinor++;
            }
        }
    }
    if (!heavyOrPawn) {
        // K vs K, or K + 1 minor vs K
        if (wPieces == 1 && bPieces == 1) return GAME_DRAW_INSUFFICIENT_MATERIAL;
        if ((wPieces == 1 && bPieces == 2 && bMinor == 1) ||
            (bPieces == 1 && wPieces == 2 && wMinor == 1)) {
            return GAME_DRAW_INSUFFICIENT_MATERIAL;
        }
    }

    // 50-move rule
    if (state && state->halfMoveClock >= 100) return GAME_DRAW_50_MOVE;

    // Checkmate / Stalemate
    if (!hasAnyLegalMove(board, state, colorToMove)) {
        if (isInCheck(board, colorToMove)) {
            return (colorToMove == 'w') ? GAME_CHECKMATE_BLACK_WINS : GAME_CHECKMATE_WHITE_WINS;
        }
        return GAME_STALEMATE;
    }

    return GAME_ONGOING;
}

void ChessEngine::applyMove(char board[8][8], GameState* state,
                            int fromRow, int fromCol, int toRow, int toCol,
                            char promoteTo) {
    char piece = board[fromRow][fromCol];
    char captured = board[toRow][toCol];
    char up = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;
    char color = getPieceColor(piece);

    bool isPawnMove = (up == 'P');
    bool isCapture = (captured != ' ');
    bool isCastle = (up == 'K' && (toCol - fromCol == 2 || toCol - fromCol == -2));
    bool isEP = false;

    if (state && isPawnMove && fromCol != toCol && captured == ' ' &&
        toRow == state->enPassantRow && toCol == state->enPassantCol) {
        // En-passant capture
        isEP = true;
        isCapture = true;
        int capturedPawnRow = (color == 'w') ? toRow - 1 : toRow + 1;
        board[capturedPawnRow][toCol] = ' ';
    }

    // Move the piece
    board[toRow][toCol] = piece;
    board[fromRow][fromCol] = ' ';

    // Promotion
    if (isPawnMove && (toRow == 7 || toRow == 0)) {
        if (promoteTo != 0) {
            // Caller specifies the case correctly (uppercase for white, lowercase for black).
            board[toRow][toCol] = promoteTo;
        } else {
            board[toRow][toCol] = (color == 'w') ? 'Q' : 'q';
        }
    }

    // Castling: also move the rook
    if (isCastle) {
        if (toCol == 6) {
            board[toRow][5] = board[toRow][7];
            board[toRow][7] = ' ';
        } else if (toCol == 2) {
            board[toRow][3] = board[toRow][0];
            board[toRow][0] = ' ';
        }
    }

    if (state) {
        // Halfmove clock
        if (isPawnMove || isCapture) state->halfMoveClock = 0;
        else state->halfMoveClock++;

        // Castling rights
        if (up == 'K') {
            if (color == 'w') { state->whiteCanCastleKingside = false; state->whiteCanCastleQueenside = false; }
            else              { state->blackCanCastleKingside = false; state->blackCanCastleQueenside = false; }
        }
        if (up == 'R') {
            if (color == 'w' && fromRow == 0) {
                if (fromCol == 0) state->whiteCanCastleQueenside = false;
                if (fromCol == 7) state->whiteCanCastleKingside = false;
            }
            if (color == 'b' && fromRow == 7) {
                if (fromCol == 0) state->blackCanCastleQueenside = false;
                if (fromCol == 7) state->blackCanCastleKingside = false;
            }
        }
        // If a rook is captured on its starting square, that side loses castling
        if (toRow == 0 && toCol == 0) state->whiteCanCastleQueenside = false;
        if (toRow == 0 && toCol == 7) state->whiteCanCastleKingside  = false;
        if (toRow == 7 && toCol == 0) state->blackCanCastleQueenside = false;
        if (toRow == 7 && toCol == 7) state->blackCanCastleKingside  = false;

        // En-passant target: set only on a 2-square pawn advance
        if (up == 'P' && (toRow - fromRow == 2 || fromRow - toRow == 2)) {
            state->enPassantRow = (fromRow + toRow) / 2;
            state->enPassantCol = fromCol;
        } else {
            state->enPassantRow = -1;
            state->enPassantCol = -1;
        }

        // Last-move tracking
        state->lastFromRow = fromRow;
        state->lastFromCol = fromCol;
        state->lastToRow = toRow;
        state->lastToCol = toCol;
        state->lastPiece = piece;
    }
}

// =============================================================
// Self-tests. Run at boot from setup(); print summary to Serial.
// All tests are pure C++ and return a fail count without touching
// the board hardware or WiFi.
// =============================================================

static void copyBoard(char dst[8][8], const char src[8][8]) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) dst[r][c] = src[r][c];
}

int ChessEngine::runSelfTests() {
    int failures = 0;
    Serial.println("=== ChessEngine self-tests ===");

    // Initial position used as a base.
    const char start[8][8] = {
        {'R','N','B','Q','K','B','N','R'},
        {'P','P','P','P','P','P','P','P'},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {'p','p','p','p','p','p','p','p'},
        {'r','n','b','q','k','b','n','r'}
    };

    // Test 1: white pawn on e2 has 2 forward moves at start
    {
        int n; int m[MAX_MOVES_PER_PIECE][2];
        getLegalMoves(start, nullptr, 1, 4, n, m);
        if (n != 2) { failures++; Serial.print("FAIL T1 e2 pawn moves="); Serial.println(n); }
        else Serial.println("PASS T1: e2 pawn has 2 legal moves");
    }

    // Test 2: white knight on b1 has 2 legal moves (a3, c3)
    {
        int n; int m[MAX_MOVES_PER_PIECE][2];
        getLegalMoves(start, nullptr, 0, 1, n, m);
        if (n != 2) { failures++; Serial.print("FAIL T2 b1 knight moves="); Serial.println(n); }
        else Serial.println("PASS T2: b1 knight has 2 legal moves");
    }

    // Test 3: at start, white is not in check, has many legal moves, game ongoing
    {
        bool check = isInCheck(start, 'w');
        if (check) { failures++; Serial.println("FAIL T3 white in check at start?!"); }
        else Serial.println("PASS T3: no check at start");
    }

    // Test 4: Fool's Mate position. After 1.f3 e5 2.g4 Qh4# white is mated.
    // White pawn pushed g2->g4 sits on (3,6); black queen now on h4 = (3,7);
    // black pawn on e5 = (4,4); white pawn f2->f3 sits on (2,5).
    {
        char b[8][8] = {
            {'R','N','B','Q','K','B','N','R'},
            {'P','P','P','P','P',' ',' ','P'},
            {' ',' ',' ',' ',' ','P',' ',' '},
            {' ',' ',' ',' ',' ',' ','P','q'},
            {' ',' ',' ',' ','p',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {'p','p','p','p',' ','p','p','p'},
            {'r','n','b',' ','k','b','n','r'}
        };
        bool check = isInCheck(b, 'w');
        bool any = hasAnyLegalMove(b, nullptr, 'w');
        if (!check) { failures++; Serial.println("FAIL T4 white not in check after Fool's Mate"); }
        if (any)    { failures++; Serial.println("FAIL T4 white has legal move after Fool's Mate"); }
        GameResult r = getGameResult(b, nullptr, 'w');
        if (r != GAME_CHECKMATE_BLACK_WINS) { failures++; Serial.print("FAIL T4 result="); Serial.println(r); }
        else Serial.println("PASS T4: Fool's Mate detected");
    }

    // Test 5: pinned piece may not move off the pin
    // White king on e1, white rook on e2, black queen on e8. The rook
    // on e2 is pinned and cannot move off the e-file.
    {
        char b[8][8] = {
            {' ',' ',' ',' ','K',' ',' ',' '},
            {' ',' ',' ',' ','R',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ','q',' ',' ',' '}
        };
        int n; int m[MAX_MOVES_PER_PIECE][2];
        getLegalMoves(b, nullptr, 1, 4, n, m); // white rook on e2
        // The rook can move along the e-file: to e3..e7 (5 squares) and capture the queen on e8 (1).
        // It cannot leave the file (would expose king).
        bool allOnFile = true;
        for (int i = 0; i < n; i++) if (m[i][1] != 4) { allOnFile = false; break; }
        if (!allOnFile) { failures++; Serial.println("FAIL T5 pinned rook left e-file"); }
        else Serial.println("PASS T5: pinned rook stayed on file");
    }

    // Test 6: white kingside castling legal in the canonical position
    {
        char b[8][8] = {
            {'R',' ',' ',' ','K',' ',' ','R'},
            {'P','P','P','P','P','P','P','P'},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {'p','p','p','p','p','p','p','p'},
            {'r','n','b','q','k','b','n','r'}
        };
        GameState st; // default: all castling allowed
        int n; int m[MAX_MOVES_PER_PIECE][2];
        getLegalMoves(b, &st, 0, 4, n, m); // white king on e1
        bool foundKingside = false, foundQueenside = false;
        for (int i = 0; i < n; i++) {
            if (m[i][0] == 0 && m[i][1] == 6) foundKingside = true;
            if (m[i][0] == 0 && m[i][1] == 2) foundQueenside = true;
        }
        if (!foundKingside)  { failures++; Serial.println("FAIL T6 kingside castle missing"); }
        if (!foundQueenside) { failures++; Serial.println("FAIL T6 queenside castle missing"); }
        if (foundKingside && foundQueenside) Serial.println("PASS T6: both castlings legal");
    }

    // Test 7: castling forbidden when king is in check
    {
        char b[8][8] = {
            {'R',' ',' ',' ','K',' ',' ','R'},
            {'P','P','P','P',' ','P','P','P'},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ','q',' ',' ',' '},  // black queen attacks e1
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {'p','p','p','p',' ','p','p','p'},
            {'r','n','b',' ','k','b','n','r'}
        };
        GameState st;
        int n; int m[MAX_MOVES_PER_PIECE][2];
        getLegalMoves(b, &st, 0, 4, n, m);
        for (int i = 0; i < n; i++) {
            if (m[i][1] == 6 || m[i][1] == 2) {
                failures++;
                Serial.print("FAIL T7 castle while in check at col="); Serial.println(m[i][1]);
            }
        }
        Serial.println("PASS T7: no castling in check (or fail above)");
    }

    // Test 8: en passant capture available
    {
        // Position after 1.e4 ... 2.e5 d5  (white to move, can EP d6)
        // Internally: white pawn just moved to (4,3) [d5 in our row=rank-1 mapping is wrong;
        // remember row 0 = rank 1 so rank 5 = row 4, and d5 = (4,3)... wait.
        // Actually internal: white pawn on row 3 = rank 4. We want a white pawn on
        // row 4 (rank 5) col 4 (e5), and a black pawn just moved to row 4 col 3 (d5),
        // so EP target is row 5 col 3 (d6).
        char b[8][8] = {
            {'R','N','B','Q','K','B','N','R'},
            {'P','P','P','P',' ','P','P','P'},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ','p','P',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {'p','p','p',' ','p','p','p','p'},
            {'r','n','b','q','k','b','n','r'}
        };
        GameState st;
        st.enPassantRow = 5;
        st.enPassantCol = 3;
        int n; int m[MAX_MOVES_PER_PIECE][2];
        getLegalMoves(b, &st, 4, 4, n, m); // white pawn on e5 = (4,4)
        bool foundEP = false;
        for (int i = 0; i < n; i++) if (m[i][0] == 5 && m[i][1] == 3) foundEP = true;
        if (!foundEP) { failures++; Serial.println("FAIL T8 en-passant move missing"); }
        else Serial.println("PASS T8: en-passant offered");
    }

    // Test 9: insufficient material draw (K vs K)
    {
        char b[8][8] = {
            {' ',' ',' ',' ','K',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ','k',' ',' ',' '}
        };
        GameResult r = getGameResult(b, nullptr, 'w');
        if (r != GAME_DRAW_INSUFFICIENT_MATERIAL) { failures++; Serial.print("FAIL T9 result="); Serial.println(r); }
        else Serial.println("PASS T9: K vs K is draw");
    }

    // Test 10: applyMove castling moves the rook too
    {
        char b[8][8] = {
            {'R',' ',' ',' ','K',' ',' ','R'},
            {'P','P','P','P','P','P','P','P'},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {' ',' ',' ',' ',' ',' ',' ',' '},
            {'p','p','p','p','p','p','p','p'},
            {'r','n','b','q','k','b','n','r'}
        };
        GameState st;
        applyMove(b, &st, 0, 4, 0, 6); // white kingside castle
        if (b[0][6] != 'K' || b[0][5] != 'R' || b[0][7] != ' ' || b[0][4] != ' ') {
            failures++;
            Serial.println("FAIL T10 castle layout wrong");
        } else Serial.println("PASS T10: kingside castle layout correct");
        if (st.whiteCanCastleKingside || st.whiteCanCastleQueenside) {
            failures++;
            Serial.println("FAIL T10 castle rights not cleared");
        }
    }

    Serial.print("=== Self-tests complete: ");
    Serial.print(10 - failures);
    Serial.print("/10 passed");
    if (failures > 0) {
        Serial.print(", ");
        Serial.print(failures);
        Serial.print(" FAILURES ");
    }
    Serial.println(" ===");
    return failures;
}
