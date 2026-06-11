#include "chess_moves.h"
#include <Arduino.h>

// Expected initial configuration (as printed in the grid)
const char ChessMoves::INITIAL_BOARD[8][8] = {
  {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'},  // row 0 (rank 1) - white back rank
  {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},  // row 1 (rank 2)
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
  {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},  // row 6 (rank 7)
  {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'}   // row 7 (rank 8) - black back rank
};

ChessMoves::ChessMoves(BoardDriver* bd, ChessEngine* ce) : boardDriver(bd), chessEngine(ce) {
    initializeBoard();
    state.reset();
    turnColor = 'w';
    gameOver = false;
}

void ChessMoves::begin() {
    Serial.println("Starting Chess Game Mode (Human vs Human)...");

    initializeBoard();
    state.reset();
    turnColor = 'w';
    gameOver = false;

    waitForBoardSetup();

    Serial.println("Chess game ready to start!");
    boardDriver->fireworkAnimation();

    // Persist the fresh game so an immediate power-off still resumes.
    persistenceSaveGame(1, 0, turnColor, board, state);

    boardDriver->readSensors();
    boardDriver->updateSensorPrev();
}

// Resume a saved game: adopt board/state/turn and skip setup + fireworks.
void ChessMoves::resumeFrom(const SavedGame& g) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            board[r][c] = g.board[r][c];
    state.reset();
    persistenceUnpackState(g, state);
    turnColor = g.turnColor;
    gameOver = false;

    Serial.print("Resumed Human-vs-Human game, ");
    Serial.print(turnColor == 'w' ? "White" : "Black");
    Serial.println(" to move.");

    boardDriver->clearAllLEDs();
    boardDriver->showLEDs();
    boardDriver->readSensors();
    boardDriver->updateSensorPrev();
}

void ChessMoves::update() {
    if (gameOver) {
        delay(500);
        return;
    }

    boardDriver->readSensors();

    // Look for a piece pickup (sensor went from active to inactive)
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (!(boardDriver->getSensorPrev(row, col) && !boardDriver->getSensorState(row, col))) continue;

            char piece = board[row][col];
            if (piece == ' ') continue;

            // Enforce turn order
            if (chessEngine->getPieceColor(piece) != turnColor) {
                Serial.print("Wrong turn: it's ");
                Serial.print(turnColor == 'w' ? "white" : "black");
                Serial.print(" to move, but lifted ");
                Serial.println(piece);
                // Brief red blink to signal wrong color, but don't block - user will replace
                boardDriver->blinkSquare(row, col, 1);
                boardDriver->updateSensorPrev();
                return;
            }

            Serial.print("Piece lifted from ");
            Serial.print((char)('a' + col));
            Serial.println(row + 1);

            // Generate LEGAL moves (filters self-check, includes castling/en-passant)
            int moveCount = 0;
            int moves[MAX_MOVES_PER_PIECE][2];
            chessEngine->getLegalMoves(board, &state, row, col, moveCount, moves);

            if (moveCount == 0) {
                Serial.println("No legal moves for this piece (pinned or no escape)");
                boardDriver->blinkSquare(row, col, 2);
                boardDriver->updateSensorPrev();
                return;
            }

            // Light up source + possible move squares. Clear first so the
            // turn-indicator breathing (a lit back rank) doesn't bleed into
            // the move display.
            boardDriver->clearAllLEDs();
            boardDriver->setSquareLED(row, col, 0, 0, 0, 100); // Soft white source
            for (int i = 0; i < moveCount; i++) {
                int r = moves[i][0];
                int c = moves[i][1];
                if (board[r][c] == ' ') {
                    // En-passant target square shows pink (capture but empty)
                    if (state.enPassantRow == r && state.enPassantCol == c &&
                        (piece == 'P' || piece == 'p')) {
                        boardDriver->setSquareLED(r, c, 200, 0, 100, 0);
                    } else {
                        boardDriver->setSquareLED(r, c, 0, 0, 0, 50);
                    }
                } else {
                    boardDriver->setSquareLED(r, c, 255, 0, 0, 50); // Red = capture
                }
            }
            boardDriver->showLEDs();

            // Wait for piece placement
            int targetRow = -1, targetCol = -1;
            bool piecePlaced = false;
            bool captureInProgress = false;

            while (!piecePlaced) {
                boardDriver->readSensors();

                // Returned to original square: cancel
                if (boardDriver->getSensorState(row, col)) {
                    targetRow = row; targetCol = col;
                    piecePlaced = true;
                    break;
                }

                for (int r2 = 0; r2 < 8 && !piecePlaced; r2++) {
                    for (int c2 = 0; c2 < 8 && !piecePlaced; c2++) {
                        if (r2 == row && c2 == col) continue;

                        bool isLegalDest = false;
                        for (int i = 0; i < moveCount; i++) {
                            if (moves[i][0] == r2 && moves[i][1] == c2) { isLegalDest = true; break; }
                        }
                        if (!isLegalDest) continue;

                        // Capture: enemy piece lifted from target
                        if (board[r2][c2] != ' ' && !boardDriver->getSensorState(r2, c2) && boardDriver->getSensorPrev(r2, c2)) {
                            Serial.print("Capture initiated at ");
                            Serial.print((char)('a' + c2));
                            Serial.println(r2 + 1);
                            targetRow = r2; targetCol = c2;
                            captureInProgress = true;
                            boardDriver->setSquareLED(r2, c2, 255, 0, 0, 100);
                            boardDriver->showLEDs();
                            // Wait for capturing piece to be placed on target
                            while (true) {
                                boardDriver->readSensors();
                                if (boardDriver->getSensorState(r2, c2)) {
                                    piecePlaced = true;
                                    break;
                                }
                                delay(50);
                            }
                            break;
                        }

                        // Quiet move: piece appeared on empty target
                        if (board[r2][c2] == ' ' && boardDriver->getSensorState(r2, c2) && !boardDriver->getSensorPrev(r2, c2)) {
                            targetRow = r2; targetCol = c2;
                            piecePlaced = true;
                            break;
                        }
                    }
                }
                delay(50);
            }

            // Cancelled (piece replaced)
            if (targetRow == row && targetCol == col) {
                Serial.println("Piece replaced - move cancelled");
                boardDriver->clearAllLEDs();
                boardDriver->updateSensorPrev();
                return;
            }

            // Apply the move via the engine (handles castling rook, EP, state updates)
            char promotedTo = 0;
            if (chessEngine->isPawnPromotion(piece, targetRow)) {
                Serial.println("Promotion! Choose your piece on the selector LEDs.");
                boardDriver->promotionAnimation(targetCol);
                promotedTo = askPromotionChoice(turnColor);
                Serial.print("Promoted to: "); Serial.println(promotedTo);
            }

            // Detect castling BEFORE applyMove so we still have the rook on
            // its original square. We'll use this to draw a blue hint to the
            // user telling them which rook to move where.
            bool wasCastle = chessEngine->isCastlingMove(board, row, col, targetRow, targetCol);
            int rookFromCol = -1, rookToCol = -1;
            if (wasCastle) {
                if (targetCol == 6) {       // kingside
                    rookFromCol = 7;
                    rookToCol = 5;
                } else if (targetCol == 2) { // queenside
                    rookFromCol = 0;
                    rookToCol = 3;
                }
            }

            bool wasCapture = (board[targetRow][targetCol] != ' ') ||
                              chessEngine->isEnPassantMove(board, &state, row, col, targetRow, targetCol);
            if (wasCapture) boardDriver->captureAnimation();

            chessEngine->applyMove(board, &state, row, col, targetRow, targetCol, promotedTo);

            // Castling visual hint: after the king has been placed, light up
            // the rook's source square in blinking blue and its destination in
            // solid blue. This makes it obvious to the user where the rook
            // physically needs to go to complete the castle.
            if (wasCastle && rookFromCol >= 0) {
                Serial.print("Castling: move the rook from ");
                Serial.print((char)('a' + rookFromCol));
                Serial.print(targetRow + 1);
                Serial.print(" to ");
                Serial.print((char)('a' + rookToCol));
                Serial.println(targetRow + 1);
                // Flash the rook source square 3x while keeping the destination
                // solid blue, then leave both lit for an extra 2 s so the user
                // has plenty of time to act.
                for (int f = 0; f < 4; f++) {
                    boardDriver->clearAllLEDs();
                    // King's destination stays soft white (it's already there)
                    boardDriver->setSquareLED(targetRow, targetCol, 0, 0, 0, 100);
                    // Rook destination = solid bright blue (where to put it)
                    boardDriver->setSquareLED(targetRow, rookToCol, 0, 0, 255);
                    // Rook source = blinking blue (where to lift from)
                    if (f % 2 == 0) {
                        boardDriver->setSquareLED(targetRow, rookFromCol, 0, 0, 255);
                    }
                    boardDriver->showLEDs();
                    delay(350);
                }
                // Hold the hint for 2 more seconds so a slow user catches it
                boardDriver->clearAllLEDs();
                boardDriver->setSquareLED(targetRow, targetCol, 0, 0, 0, 100);
                boardDriver->setSquareLED(targetRow, rookFromCol, 0, 0, 200);
                boardDriver->setSquareLED(targetRow, rookToCol, 0, 0, 255);
                boardDriver->showLEDs();
                delay(2000);
            }

            // Confirmation: double blink dest
            for (int b = 0; b < 2; b++) {
                boardDriver->setSquareLED(targetRow, targetCol, 0, 0, 0, 255);
                boardDriver->showLEDs();
                delay(150);
                boardDriver->setSquareLED(targetRow, targetCol, 0, 0, 0, 50);
                boardDriver->showLEDs();
                delay(150);
            }
            boardDriver->clearAllLEDs();

            // Switch turn
            turnColor = (turnColor == 'w') ? 'b' : 'w';

            // Persist the new position so the game survives a power cycle.
            // (mode 1 = Human-vs-Human; difficulty unused here.)
            persistenceSaveGame(1, 0, turnColor, board, state);

            // Game over?
            GameResult result = chessEngine->getGameResult(board, &state, turnColor);
            if (result != GAME_ONGOING) {
                announceGameResult(result);
                gameOver = true;
                persistenceClear(); // game finished, nothing to resume
                boardDriver->updateSensorPrev();
                return;
            }

            // Check announcement
            if (chessEngine->isInCheck(board, turnColor)) {
                Serial.print(turnColor == 'w' ? "White" : "Black");
                Serial.println(" is in CHECK!");
                blinkKingInCheck(turnColor);
            }

            boardDriver->updateSensorPrev();
            return;
        }
    }

    // Idle (no piece lifted this scan): breathe the side-to-move's back rank
    // so it's always obvious whose turn it is. White = rank 1 (row 0) in soft
    // white; Black = rank 8 (row 7) in red.
    if (turnColor == 'w') {
        boardDriver->breatheRow(0, 0, 0, 0, 200); // white channel
    } else {
        boardDriver->breatheRow(7, 220, 0, 0, 0); // red
    }

    boardDriver->updateSensorPrev();
}

void ChessMoves::initializeBoard() {
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 8; col++)
            board[row][col] = INITIAL_BOARD[row][col];
}

void ChessMoves::waitForBoardSetup() {
    Serial.println("Waiting for pieces to be placed...");
    while (!boardDriver->checkInitialBoard(INITIAL_BOARD)) {
        boardDriver->updateSetupDisplay(INITIAL_BOARD);
        delay(200);
    }
}

void ChessMoves::processMove(int fromRow, int fromCol, int toRow, int toCol, char piece) {
    // Legacy entrypoint kept for compatibility; new code uses applyMove via engine.
    board[toRow][toCol] = piece;
    board[fromRow][fromCol] = ' ';
}

void ChessMoves::checkForPromotion(int targetRow, int targetCol, char piece) {
    // Legacy stub - promotion is now handled inline in update() before applyMove.
    (void)targetRow; (void)targetCol; (void)piece;
}

void ChessMoves::handlePromotion(int targetRow, int targetCol, char piece) {
    (void)targetRow; (void)targetCol; (void)piece;
}

// Show 4 selector LEDs in the back rank (a1/b1/c1/d1 for white,
// a8/b8/c8/d8 for black) representing Q/R/B/N. The user places any
// chess piece on the desired square. We pick the first sensor that
// goes active among the four selector squares.
char ChessMoves::askPromotionChoice(char color) {
    int row = (color == 'w') ? 0 : 7;
    // Light selector squares with distinct colors:
    // a-file = Q (gold), b-file = R (red), c-file = B (blue), d-file = N (white)
    boardDriver->clearAllLEDs();
    boardDriver->setSquareLED(row, 0, 255, 215, 0,   0); // Q: gold
    boardDriver->setSquareLED(row, 1, 255, 0,   0,   0); // R: red
    boardDriver->setSquareLED(row, 2, 0,   0,   255, 0); // B: blue
    boardDriver->setSquareLED(row, 3, 0,   0,   0,  255); // N: white
    boardDriver->showLEDs();

    // First flush the sensor state for these 4 squares
    boardDriver->readSensors();
    boardDriver->updateSensorPrev();

    char selected = 0;
    while (selected == 0) {
        boardDriver->readSensors();
        // Pick the first selector that transitioned to active
        for (int c = 0; c < 4; c++) {
            if (boardDriver->getSensorState(row, c) && !boardDriver->getSensorPrev(row, c)) {
                switch (c) {
                    case 0: selected = (color == 'w') ? 'Q' : 'q'; break;
                    case 1: selected = (color == 'w') ? 'R' : 'r'; break;
                    case 2: selected = (color == 'w') ? 'B' : 'b'; break;
                    case 3: selected = (color == 'w') ? 'N' : 'n'; break;
                }
                break;
            }
        }
        boardDriver->updateSensorPrev();
        delay(50);
    }

    // Wait for the user to lift the selector piece off the menu so we
    // don't immediately treat the next loop iteration as a 'move'.
    while (boardDriver->getSensorState(row, 0) || boardDriver->getSensorState(row, 1) ||
           boardDriver->getSensorState(row, 2) || boardDriver->getSensorState(row, 3)) {
        boardDriver->readSensors();
        delay(50);
    }
    boardDriver->clearAllLEDs();
    return selected;
}

void ChessMoves::announceGameResult(GameResult r) {
    boardDriver->clearAllLEDs();
    switch (r) {
        case GAME_CHECKMATE_WHITE_WINS:
            Serial.println("CHECKMATE - WHITE WINS");
            Serial.println("Fireworks until a piece is lifted...");
            boardDriver->celebrateUntilLifted();
            break;
        case GAME_CHECKMATE_BLACK_WINS:
            Serial.println("CHECKMATE - BLACK WINS");
            Serial.println("Fireworks until a piece is lifted...");
            boardDriver->celebrateUntilLifted();
            break;
        case GAME_STALEMATE:
            Serial.println("STALEMATE - Draw");
            boardDriver->fireworkAnimation();
            break;
        case GAME_DRAW_50_MOVE:
            Serial.println("DRAW - 50-move rule");
            boardDriver->fireworkAnimation();
            break;
        case GAME_DRAW_INSUFFICIENT_MATERIAL:
            Serial.println("DRAW - Insufficient material");
            boardDriver->fireworkAnimation();
            break;
        default: break;
    }
    Serial.println("Reset the board (lift all pieces) to play again.");
}

void ChessMoves::blinkKingInCheck(char color) {
    char target = (color == 'w') ? 'K' : 'k';
    int kr = -1, kc = -1;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == target) { kr = r; kc = c; }
    if (kr < 0) return;
    for (int b = 0; b < 3; b++) {
        boardDriver->setSquareLED(kr, kc, 255, 0, 0, 0);
        boardDriver->showLEDs(); delay(150);
        boardDriver->setSquareLED(kr, kc, 0, 0, 0, 0);
        boardDriver->showLEDs(); delay(150);
    }
}

bool ChessMoves::isActive() {
    return !gameOver;
}

void ChessMoves::reset() {
    boardDriver->clearAllLEDs();
    initializeBoard();
    state.reset();
    turnColor = 'w';
    gameOver = false;
}
