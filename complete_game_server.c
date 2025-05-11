#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 8081
#define MAX 256
#define BUFFER_SIZE 2048
#define MAX_CLIENTS 10
#define SA struct sockaddr

// Wordle
const char *wordList[] = {"APPLE", "GRAPE", "MANGO", "BERRY", "LEMON", "WATER", "ORBIT"};
const int wordListSize = 7;

// Chess
typedef enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;
typedef enum { WHITE, BLACK } Color;

typedef struct {
    PieceType type;
    Color color;
    char id[4];
} Piece;

typedef struct {
    Piece* board[8][8];
} ChessBoard;

// Snake and Ladder
typedef struct {
    int start;
    int end;
} SnakeLadder;

SnakeLadder snakes[] = {{16,6}, {47,26}, {49,11}, {56,53}, {62,19}, {64,60}, {87,24}, {93,73}, {95,75}, {98,78}};
int num_snakes = 10;

SnakeLadder ladders[] = {{1,38}, {4,14}, {9,31}, {21,42}, {28,84}, {36,44}, {51,67}, {71,91}, {80,100}};
int num_ladders = 9;

// Game Session
typedef enum { WORDLE, CHESS, SNAKE_LADDER, TIC_TAC_TOE, ROCK_PAPER_SCISSOR } GameType;

typedef struct {
    int connfd;
    char gameChoice[20];
} WaitingPlayer;

typedef struct {
    int player1_fd;
    int player2_fd;
    GameType gameType;
    int gameOver;
    // Wordle
    char secretWord[6];
    int turn;
    int p1Attempts;
    int p2Attempts;
    int maxAttempts;
    // Chess
    ChessBoard chessBoard;
    int chessTurn;
    enum { WAITING, PLAYING, FINISHED } chessState;
    // Snake and Ladder
    int slPositions[2];
    int slTurn;
    enum { SL_WAITING, SL_PLAYING, SL_FINISHED } slState;
    // Tic Tac Toe
    char tttBoard[3][3];
    char tttCurrentPlayer;
    int tttTurn;
    // Rock Paper Scissors
    int rpsScore[2];
    int rpsRounds;
} GameSession;

WaitingPlayer waitingPlayers[MAX_CLIENTS];
int numWaiting = 0;
GameSession sessions[MAX_CLIENTS];
int numSessions = 0;

// Utility Functions
void send_full(int connfd, const char *msg) {
    size_t len = strlen(msg);
    size_t total = 0;
    while (total < len) {
        int sent = write(connfd, msg + total, len - total);
        if (sent <= 0) break;
        total += sent;
    }
}

void send_to_player(int connfd, const char *msg) {
    send_full(connfd, msg);
}

void broadcast(GameSession *session, const char *msg) {
    send_to_player(session->player1_fd, msg);
    send_to_player(session->player2_fd, msg);
}

// Wordle Functions
void checkGuess(const char *guess, const char *secret, char *feedback) {
    for (int i = 0; i < 5; i++) {
        if (guess[i] == secret[i]) feedback[i] = guess[i];
        else feedback[i] = '*';
    }
    feedback[5] = '\0';
}

int receiveGuess(int sockfd, char *guess) {
    char buff[MAX];
    bzero(buff, MAX);
    read(sockfd, buff, sizeof(buff));
    if (strncmp(buff, "exit", 4) == 0) return 0;
    strncpy(guess, buff, 6);
    guess[5] = '\0';
    return 1;
}

void runWordleGame(GameSession *session) {
    char guess[6], feedback[6];
    int current_fd;
    char msg[MAX];

    while (!session->gameOver) {
        current_fd = (session->turn == 1) ? session->player1_fd : session->player2_fd;
        int *currentAttempts = (session->turn == 1) ? &session->p1Attempts : &session->p2Attempts;
        char playerName[10];
        snprintf(playerName, sizeof(playerName), "Player %d", session->turn);

        snprintf(msg, MAX, "Your turn, %s. Enter a 5-letter guess:\n", playerName);
        send_to_player(current_fd, msg);

        int other_fd = (session->turn == 1) ? session->player2_fd : session->player1_fd;
        snprintf(msg, MAX, "Waiting for %s to guess...\n", playerName);
        send_to_player(other_fd, msg);

        if (!receiveGuess(current_fd, guess)) {
            snprintf(msg, MAX, "%s disconnected. Game over.\n", playerName);
            broadcast(session, msg);
            session->gameOver = 1;
            break;
        }

        if (strlen(guess) != 5) {
            send_to_player(current_fd, "Invalid guess! Must be 5 letters.\n");
            continue;
        }

        for (int i = 0; i < 5; i++) {
            if (guess[i] >= 'a' && guess[i] <= 'z') guess[i] -= 32;
        }

        checkGuess(guess, session->secretWord, feedback);
        (*currentAttempts)++;

        snprintf(msg, MAX, "%s guessed: %s, Feedback: %s\n", playerName, guess, feedback);
        broadcast(session, msg);

        if (strcmp(guess, session->secretWord) == 0) {
            snprintf(msg, MAX, "%s wins! The word was: %s\n", playerName, session->secretWord);
            broadcast(session, msg);
            session->gameOver = 1;
            break;
        }

        if (session->p1Attempts >= session->maxAttempts && session->p2Attempts >= session->maxAttempts) {
            snprintf(msg, MAX, "Game over! No one guessed the word: %s\n", session->secretWord);
            broadcast(session, msg);
            session->gameOver = 1;
            break;
        }

        session->turn = (session->turn == 1) ? 2 : 1;
    }
}

// Chess Functions
void init_chess_board(ChessBoard* board) {
    for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) board->board[i][j] = NULL;
    board->board[7][0] = (Piece*)malloc(sizeof(Piece)); *board->board[7][0] = (Piece){ROOK, WHITE, "R1W"};
    board->board[7][1] = (Piece*)malloc(sizeof(Piece)); *board->board[7][1] = (Piece){KNIGHT, WHITE, "K1W"};
    board->board[7][2] = (Piece*)malloc(sizeof(Piece)); *board->board[7][2] = (Piece){BISHOP, WHITE, "B1W"};
    board->board[7][3] = (Piece*)malloc(sizeof(Piece)); *board->board[7][3] = (Piece){QUEEN, WHITE, "QW"};
    board->board[7][4] = (Piece*)malloc(sizeof(Piece)); *board->board[7][4] = (Piece){KING, WHITE, "KW"};
    board->board[7][5] = (Piece*)malloc(sizeof(Piece)); *board->board[7][5] = (Piece){BISHOP, WHITE, "B2W"};
    board->board[7][6] = (Piece*)malloc(sizeof(Piece)); *board->board[7][6] = (Piece){KNIGHT, WHITE, "K2W"};
    board->board[7][7] = (Piece*)malloc(sizeof(Piece)); *board->board[7][7] = (Piece){ROOK, WHITE, "R2W"};
    for (int i = 0; i < 8; i++) {
        board->board[6][i] = (Piece*)malloc(sizeof(Piece));
        sprintf(board->board[6][i]->id, "P%dW", i + 1);
        board->board[6][i]->type = PAWN;
        board->board[6][i]->color = WHITE;
    }
    board->board[0][0] = (Piece*)malloc(sizeof(Piece)); *board->board[0][0] = (Piece){ROOK, BLACK, "R1B"};
    board->board[0][1] = (Piece*)malloc(sizeof(Piece)); *board->board[0][1] = (Piece){KNIGHT, BLACK, "K1B"};
    board->board[0][2] = (Piece*)malloc(sizeof(Piece)); *board->board[0][2] = (Piece){BISHOP, BLACK, "B1B"};
    board->board[0][3] = (Piece*)malloc(sizeof(Piece)); *board->board[0][3] = (Piece){QUEEN, BLACK, "QB"};
    board->board[0][4] = (Piece*)malloc(sizeof(Piece)); *board->board[0][4] = (Piece){KING, BLACK, "KB"};
    board->board[0][5] = (Piece*)malloc(sizeof(Piece)); *board->board[0][5] = (Piece){BISHOP, BLACK, "B2B"};
    board->board[0][6] = (Piece*)malloc(sizeof(Piece)); *board->board[0][6] = (Piece){KNIGHT, BLACK, "K2B"};
    board->board[0][7] = (Piece*)malloc(sizeof(Piece)); *board->board[0][7] = (Piece){ROOK, BLACK, "R2B"};
    for (int i = 0; i < 8; i++) {
        board->board[1][i] = (Piece*)malloc(sizeof(Piece));
        sprintf(board->board[1][i]->id, "P%dB", i + 1);
        board->board[1][i]->type = PAWN;
        board->board[1][i]->color = BLACK;
    }
}

void free_chess_board(ChessBoard* board) {
    for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
        free(board->board[i][j]);
        board->board[i][j] = NULL;
    }
}

void get_chess_board_string(ChessBoard* board, char* board_str) {
    char* p = board_str;
    p += sprintf(p, "\n\033[1;34m✨ CHESS BOARD ✨\033[0m\n");
    p += sprintf(p, "    a   b   c   d   e   f   g   h\n");
    p += sprintf(p, "  ┌───┬───┬───┬───┬───┬───┬───┬───┐\n");
    for (int i = 0; i < 8; i++) {
        p += sprintf(p, "\033[1;34m%d\033[0m │", 8 - i);
        for (int j = 0; j < 8; j++) {
            if (board->board[i][j]) {
                char* color = board->board[i][j]->color == WHITE ? "\033[1;37m" : "\033[1;30m";
                p += sprintf(p, "%s%-3s\033[0m│", color, board->board[i][j]->id);
            } else {
                p += sprintf(p, " . │");
            }
        }
        p += sprintf(p, "\n");
        if (i < 7) p += sprintf(p, "  ├───┼───┼───┼───┼───┼───┼───┼───┤\n");
    }
    p += sprintf(p, "  └───┴───┴───┴───┴───┴───┴───┴───┘\n");
    p += sprintf(p, "    a   b   c   d   e   f   g   h\n\n");
}

void send_chess_board(GameSession *session) {
    char board_str[BUFFER_SIZE];
    bzero(board_str, BUFFER_SIZE);
    get_chess_board_string(&session->chessBoard, board_str);
    broadcast(session, board_str);
}

int is_path_clear(ChessBoard* board, int fromX, int fromY, int toX, int toY) {
    int dx = toX - fromX;
    int dy = toY - fromY;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    int stepX = dx ? dx / abs(dx) : 0;
    int stepY = dy ? dy / abs(dy) : 0;
    for (int i = 1; i < steps; i++) {
        int x = fromX + i * stepX;
        int y = fromY + i * stepY;
        if (board->board[x][y]) return 0;
    }
    return 1;
}

int is_legal_move(ChessBoard* board, int fromX, int fromY, int toX, int toY, char* feedback) {
    Piece* piece = board->board[fromX][fromY];
    int dx = toX - fromX;
    int dy = toY - fromY;
    if (fromX == toX && fromY == toY) {
        strcpy(feedback, "\033[1;31mInvalid move! Cannot move to the same square.\033[0m");
        return 0;
    }
    if (board->board[toX][toY] && board->board[toX][toY]->color == piece->color) {
        strcpy(feedback, "\033[1;31mInvalid move! Cannot capture your own piece.\033[0m");
        return 0;
    }
    switch (piece->type) {
        case PAWN:
            if (piece->color == WHITE) {
                if (dx == -1 && dy == 0 && !board->board[toX][toY]) return 1;
                if (fromX == 6 && dx == -2 && dy == 0 && !board->board[toX][toY] && !board->board[fromX - 1][fromY]) return 1;
                if (dx == -1 && (dy == 1 || dy == -1) && board->board[toX][toY] && board->board[toX][toY]->color == BLACK) return 1;
                strcpy(feedback, "\033[1;31mInvalid pawn move! White pawns move up one (or two from row 2) or capture diagonally.\033[0m");
            } else {
                if (dx == 1 && dy == 0 && !board->board[toX][toY]) return 1;
                if (fromX == 1 && dx == 2 && dy == 0 && !board->board[toX][toY] && !board->board[fromX + 1][fromY]) return 1;
                if (dx == 1 && (dy == 1 || dy == -1) && board->board[toX][toY] && board->board[toX][toY]->color == WHITE) return 1;
                strcpy(feedback, "\033[1;31mInvalid pawn move! Black pawns move down one (or two from row 7) or capture diagonally.\033[0m");
            }
            return 0;
        case KNIGHT:
            if ((abs(dx) == 2 && abs(dy) == 1) || (abs(dx) == 1 && abs(dy) == 2)) return 1;
            strcpy(feedback, "\033[1;31mInvalid knight move! Knights move in an L-shape (2x1 or 1x2).\033[0m");
            return 0;
        case BISHOP:
            if (abs(dx) == abs(dy) && is_path_clear(board, fromX, fromY, toX, toY)) return 1;
            strcpy(feedback, "\033[1;31mInvalid bishop move! Bishops move diagonally any distance.\033[0m");
            return 0;
        case ROOK:
            if ((dx == 0 || dy == 0) && is_path_clear(board, fromX, fromY, toX, toY)) return 1;
            strcpy(feedback, "\033[1;31mInvalid rook move! Rooks move horizontally or vertically any distance.\033[0m");
            return 0;
        case QUEEN:
            if ((abs(dx) == abs(dy) || dx == 0 || dy == 0) && is_path_clear(board, fromX, fromY, toX, toY)) return 1;
            strcpy(feedback, "\033[1;31mInvalid queen move! Queens move diagonally, horizontally, or vertically any distance.\033[0m");
            return 0;
        case KING:
            if (abs(dx) <= 1 && abs(dy) <= 1) return 1;
            strcpy(feedback, "\033[1;31mInvalid king move! Kings move one square in any direction.\033[0m");
            return 0;
    }
    return 0;
}

int move_piece(ChessBoard* board, const char* pieceId, const char* to, Color playerColor, char* feedback) {
    int fromX = -1, fromY = -1;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (board->board[i][j] && strcmp(board->board[i][j]->id, pieceId) == 0 && board->board[i][j]->color == playerColor) {
                fromX = i;
                fromY = j;
                break;
            }
        }
        if (fromX != -1) break;
    }
    if (fromX == -1) {
        strcpy(feedback, "\033[1;31mPiece not found or not yours!\033[0m");
        return 0;
    }
    if (strlen(to) != 2) {
        strcpy(feedback, "\033[1;31mInvalid destination format! Use e.g., 'e5'\033[0m");
        return 0;
    }
    int toY = to[0] - 'a';
    int toX = 8 - (to[1] - '0');
    if (toX < 0 || toX > 7 || toY < 0 || toY > 7) {
        strcpy(feedback, "\033[1;31mDestination out of bounds!\033[0m");
        return 0;
    }
    if (!is_legal_move(board, fromX, fromY, toX, toY, feedback)) return 0;
    Piece* movingPiece = board->board[fromX][fromY];
    Piece* targetPiece = board->board[toX][toY];
    if (movingPiece->type == PAWN && targetPiece && targetPiece->type == KING) {
        free(board->board[toX][toY]);
        board->board[toX][toY] = board->board[fromX][fromY];
        board->board[fromX][fromY] = NULL;
        strcpy(feedback, "\033[1;32mMove successful: Pawn captured King!\033[0m");
        return 2;
    }
    free(board->board[toX][toY]);
    board->board[toX][toY] = board->board[fromX][fromY];
    board->board[fromX][fromY] = NULL;
    strcpy(feedback, "\033[1;32mMove successful\033[0m");
    return 1;
}

int check_chess_winner(ChessBoard* board) {
    int whiteKing = 0, blackKing = 0;
    for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
        if (board->board[i][j] && board->board[i][j]->type == KING) {
            if (board->board[i][j]->color == WHITE) whiteKing = 1;
            else blackKing = 1;
        }
    }
    if (!whiteKing) return 1;
    if (!blackKing) return 0;
    return -1;
}

void runChessGame(GameSession *session) {
    init_chess_board(&session->chessBoard);
    session->chessState = PLAYING;
    session->chessTurn = 0;
    char msg[50];
    snprintf(msg, 50, "\033[1;33m🎉 CHESS GAME STARTED! 🎉\033[0m\n");
    broadcast(session, msg);
    send_chess_board(session);
    send_to_player(session->player1_fd, "TURN\n");

    while (!session->gameOver) {
        int current_fd = session->chessTurn == 0 ? session->player1_fd : session->player2_fd;
        char buff[BUFFER_SIZE];
        int n = read(current_fd, buff, sizeof(buff));
        if (n <= 0) {
            broadcast(session, "\033[1;31mGame ended: Player disconnected\033[0m\n");
            session->gameOver = 1;
            free_chess_board(&session->chessBoard);
            break;
        }
        buff[n] = '\0';
        if (strncmp(buff, "MOVE:", 5) == 0 && session->chessState == PLAYING) {
            char pieceId[4], to[3];
            sscanf(buff + 5, "%s %s", pieceId, to);
            char feedback[100];
            Color playerColor = session->chessTurn == 0 ? WHITE : BLACK;
            int moveResult = move_piece(&session->chessBoard, pieceId, to, playerColor, feedback);
            if (moveResult > 0) {
                char move_msg[50];
                snprintf(move_msg, 50, "\033[1;36mMOVE:Player %d (%c) moved %s to %s\033[0m\n", 
                        session->chessTurn + 1, session->chessTurn == 0 ? 'W' : 'B', pieceId, to);
                broadcast(session, move_msg);
                broadcast(session, "BOARD_UPDATE\n");
                send_chess_board(session);
                if (moveResult == 2) {
                    char win_msg[50];
                    snprintf(win_msg, 50, "\033[1;32mWINNER:Player %d (%c) by pawn capturing king!\033[0m\n", 
                            session->chessTurn + 1, session->chessTurn == 0 ? 'W' : 'B');
                    broadcast(session, win_msg);
                    session->gameOver = 1;
                    free_chess_board(&session->chessBoard);
                    break;
                }
                int winner = check_chess_winner(&session->chessBoard);
                if (winner >= 0) {
                    char win_msg[50];
                    snprintf(win_msg, 50, "\033[1;32mWINNER:Player %d (%c) by capturing king!\033[0m\n", 
                            winner + 1, winner == 0 ? 'W' : 'B');
                    broadcast(session, win_msg);
                    session->gameOver = 1;
                    free_chess_board(&session->chessBoard);
                    break;
                }
                session->chessTurn = (session->chessTurn + 1) % 2;
                int next_fd = session->chessTurn == 0 ? session->player1_fd : session->player2_fd;
                send_to_player(next_fd, "TURN\n");
            } else {
                send_to_player(current_fd, feedback);
                send_to_player(current_fd, "\nTURN\n");
            }
        }
    }
}

// Snake and Ladder Functions
void send_sl_board(GameSession *session) {
    char board_msg[BUFFER_SIZE] = "BOARD:";
    for (int i = 1; i <= 100; i++) {
        char marker = ' ';
        for (int j = 0; j < num_snakes; j++) if (snakes[j].start == i) marker = 'S';
        for (int j = 0; j < num_ladders; j++) if (ladders[j].start == i) marker = 'L';
        char temp[5];
        snprintf(temp, 5, "%d%c,", i, marker);
        strcat(board_msg, temp);
    }
    board_msg[strlen(board_msg) - 1] = '\n';
    broadcast(session, board_msg);
}

void send_sl_positions(GameSession *session) {
    char pos_msg[BUFFER_SIZE] = "POSITIONS:";
    for (int i = 0; i < 2; i++) {
        char temp[20];
        snprintf(temp, 20, "P%d=%d,", i + 1, session->slPositions[i]);
        strcat(pos_msg, temp);
    }
    pos_msg[strlen(pos_msg) - 1] = '\n';
    broadcast(session, pos_msg);
}

void runSnakeLadderGame(GameSession *session) {
    session->slPositions[0] = 0;
    session->slPositions[1] = 0;
    session->slState = SL_PLAYING;
    session->slTurn = 0;
    broadcast(session, "\033[1;33m🎉 SNAKE AND LADDER GAME STARTED! 🎉\033[0m\n");
    send_sl_board(session);
    send_sl_positions(session);
    send_to_player(session->player1_fd, "TURN\n");

    while (!session->gameOver) {
        int current_fd = session->slTurn == 0 ? session->player1_fd : session->player2_fd;
        char buff[BUFFER_SIZE];
        int n = read(current_fd, buff, sizeof(buff));
        if (n <= 0) {
            broadcast(session, "Game ended due to disconnection\n");
            session->gameOver = 1;
            break;
        }
        buff[n] = '\0';
        if (strncmp(buff, "ROLL", 4) == 0 && session->slState == SL_PLAYING) {
            int roll = rand() % 6 + 1;
            char roll_msg[50];
            snprintf(roll_msg, 50, "ROLLED:P%d=%d\n", session->slTurn + 1, roll);
            broadcast(session, roll_msg);
            int new_pos = session->slPositions[session->slTurn] + roll;
            if (new_pos <= 100) {
                session->slPositions[session->slTurn] = new_pos;
                for (int j = 0; j < num_snakes; j++) {
                    if (new_pos == snakes[j].start) {
                        session->slPositions[session->slTurn] = snakes[j].end;
                        char snake_msg[50];
                        snprintf(snake_msg, 50, "SNAKE:P%d=%d-%d\n", session->slTurn + 1, new_pos, snakes[j].end);
                        broadcast(session, snake_msg);
                        break;
                    }
                }
                for (int j = 0; j < num_ladders; j++) {
                    if (new_pos == ladders[j].start) {
                        session->slPositions[session->slTurn] = ladders[j].end;
                        char ladder_msg[50];
                        snprintf(ladder_msg, 50, "LADDER:P%d=%d-%d\n", session->slTurn + 1, new_pos, ladders[j].end);
                        broadcast(session, ladder_msg);
                        break;
                    }
                }
                if (session->slPositions[session->slTurn] >= 100) {
                    char win_msg[50];
                    snprintf(win_msg, 50, "WINNER:Player %d\n", session->slTurn + 1);
                    broadcast(session, win_msg);
                    session->gameOver = 1;
                    break;
                }
            }
            send_sl_positions(session);
            session->slTurn = (session->slTurn + 1) % 2;
            int next_fd = session->slTurn == 0 ? session->player1_fd : session->player2_fd;
            send_to_player(next_fd, "TURN\n");
        }
    }
}

// Tic Tac Toe Functions
void init_ttt_board(GameSession *session) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            session->tttBoard[i][j] = ' ';
}

void get_ttt_board_display(GameSession *session, char *buffer) {
    sprintf(buffer,
        "\n %c | %c | %c \n---|---|---\n %c | %c | %c \n---|---|---\n %c | %c | %c \n",
        session->tttBoard[0][0], session->tttBoard[0][1], session->tttBoard[0][2],
        session->tttBoard[1][0], session->tttBoard[1][1], session->tttBoard[1][2],
        session->tttBoard[2][0], session->tttBoard[2][1], session->tttBoard[2][2]);
}

int check_ttt_winner(GameSession *session) {
    for (int i = 0; i < 3; i++) {
        if (session->tttBoard[i][0] == session->tttCurrentPlayer && session->tttBoard[i][1] == session->tttCurrentPlayer && session->tttBoard[i][2] == session->tttCurrentPlayer)
            return 1;
        if (session->tttBoard[0][i] == session->tttCurrentPlayer && session->tttBoard[1][i] == session->tttCurrentPlayer && session->tttBoard[2][i] == session->tttCurrentPlayer)
            return 1;
    }
    if (session->tttBoard[0][0] == session->tttCurrentPlayer && session->tttBoard[1][1] == session->tttCurrentPlayer && session->tttBoard[2][2] == session->tttCurrentPlayer)
        return 1;
    if (session->tttBoard[0][2] == session->tttCurrentPlayer && session->tttBoard[1][1] == session->tttCurrentPlayer && session->tttBoard[2][0] == session->tttCurrentPlayer)
        return 1;
    return 0;
}

int is_ttt_draw(GameSession *session) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (session->tttBoard[i][j] == ' ') return 0;
    return 1;
}

void broadcast_ttt_board(GameSession *session) {
    char buffer[1024];
    get_ttt_board_display(session, buffer);
    broadcast(session, buffer);
}

void runTicTacToeGame(GameSession *session) {
    init_ttt_board(session);
    session->tttCurrentPlayer = 'X';
    session->tttTurn = 0;
    broadcast(session, "\033[1;33m🎉 TIC TAC TOE GAME STARTED! 🎉\033[0m\n");
    broadcast_ttt_board(session);

    while (!session->gameOver) {
        int player = session->tttTurn % 2;
        int current_fd = player == 0 ? session->player1_fd : session->player2_fd;
        char move_prompt[64];
        sprintf(move_prompt, "Your turn Player %c. Enter row and col (0-2 0-2):\n", (player == 0 ? 'X' : 'O'));
        send_to_player(current_fd, move_prompt);

        int valid_move = 0;
        while (!valid_move) {
            char buffer[1024];
            int n = read(current_fd, buffer, sizeof(buffer));
            if (n <= 0) {
                broadcast(session, "Player disconnected.\n");
                session->gameOver = 1;
                break;
            }
            buffer[n] = '\0';
            int row, col;
            if (sscanf(buffer, "%d %d", &row, &col) != 2 ||
                row < 0 || row > 2 || col < 0 || col > 2 || session->tttBoard[row][col] != ' ') {
                send_to_player(current_fd, "Invalid move. Try again (format: row col):\n");
                continue;
            }
            valid_move = 1;
            session->tttCurrentPlayer = (player == 0 ? 'X' : 'O');
            session->tttBoard[row][col] = session->tttCurrentPlayer;
        }
        if (session->gameOver) break;

        broadcast_ttt_board(session);
        if (check_ttt_winner(session)) {
            char buffer[64];
            sprintf(buffer, "Player %c wins!\n", session->tttCurrentPlayer);
            broadcast(session, buffer);
            session->gameOver = 1;
            break;
        }
        if (is_ttt_draw(session)) {
            broadcast(session, "It's a draw!\n");
            session->gameOver = 1;
            break;
        }
        session->tttTurn++;
    }
}

// Rock Paper Scissors Functions
int receive_rps_move(int sockfd, char *move, const char *playerName) {
    char buff[MAX];
    bzero(buff, MAX);
    read(sockfd, buff, sizeof(buff));
    if (strncmp(buff, "exit", 4) == 0) {
        snprintf(move, 16, "exit");
        return 0;
    }
    strncpy(move, buff, 15);
    move[15] = '\0';
    return 1;
}

const char* get_rps_winner(const char *p1, const char *p2) {
    if (strcmp(p1, p2) == 0) return "It's a tie!";
    if ((strcmp(p1, "STONE") == 0 && strcmp(p2, "SCISSORS") == 0) ||
        (strcmp(p1, "PAPER") == 0 && strcmp(p2, "STONE") == 0) ||
        (strcmp(p1, "SCISSORS") == 0 && strcmp(p2, "PAPER") == 0)) {
        return "Player 1 wins!";
    } else {
        return "Player 2 wins!";
    }
}

void runRockPaperScissorGame(GameSession *session) {
    int bestOf = 3;
    int roundsNeededToWin = (bestOf / 2) + 1;
    session->rpsScore[0] = 0;
    session->rpsScore[1] = 0;
    session->rpsRounds = 0;
    char p1Move[16], p2Move[16];
    char msg[MAX];

    broadcast(session, "\033[1;33m🎉 ROCK PAPER SCISSORS GAME STARTED! 🎉\033[0m\n");

    while (session->rpsScore[0] < roundsNeededToWin && session->rpsScore[1] < roundsNeededToWin) {
        session->rpsRounds++;
        snprintf(msg, MAX, "\n--- Round %d ---\nEnter STONE, PAPER, or SCISSORS:\n", session->rpsRounds);
        broadcast(session, msg);

        if (!receive_rps_move(session->player1_fd, p1Move, "Player 1") || 
            !receive_rps_move(session->player2_fd, p2Move, "Player 2")) {
            broadcast(session, "A player disconnected. Game over.\n");
            session->gameOver = 1;
            break;
        }

        for (int i = 0; p1Move[i]; i++) p1Move[i] = toupper(p1Move[i]);
        for (int i = 0; p2Move[i]; i++) p2Move[i] = toupper(p2Move[i]);

        snprintf(msg, MAX, "Player 1 chose: %s\n", p1Move);
        broadcast(session, msg);
        snprintf(msg, MAX, "Player 2 chose: %s\n", p2Move);
        broadcast(session, msg);

        const char *result = get_rps_winner(p1Move, p2Move);
        snprintf(msg, MAX, "Result: %s\n", result);
        broadcast(session, msg);

        if (strstr(result, "Player 1 wins")) session->rpsScore[0]++;
        else if (strstr(result, "Player 2 wins")) session->rpsScore[1]++;

        snprintf(msg, MAX, "Score: Player 1 [%d] - Player 2 [%d]\n", session->rpsScore[0], session->rpsScore[1]);
        broadcast(session, msg);
    }

    if (!session->gameOver) {
        if (session->rpsScore[0] > session->rpsScore[1]) 
            broadcast(session, "\n🏆 Player 1 wins the game!\n");
        else 
            broadcast(session, "\n🏆 Player 2 wins the game!\n");
        broadcast(session, "Game over. Thanks for playing!\n");
        session->gameOver = 1;
    }
}

// Main Server Logic
int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    fd_set readfds;
    srand(time(NULL));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed...\n");
        exit(0);
    }
    printf("Socket successfully created..\n");

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Socket bind failed...\n");
        exit(0);
    }
    printf("Socket successfully bound..\n");

    if (listen(sockfd, 5) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    printf("Server listening..\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        int max_fd = sockfd;

        for (int i = 0; i < numWaiting; i++) {
            FD_SET(waitingPlayers[i].connfd, &readfds);
            if (waitingPlayers[i].connfd > max_fd) max_fd = waitingPlayers[i].connfd;
        }
        for (int i = 0; i < numSessions; i++) {
            if (!sessions[i].gameOver) {
                FD_SET(sessions[i].player1_fd, &readfds);
                FD_SET(sessions[i].player2_fd, &readfds);
                if (sessions[i].player1_fd > max_fd) max_fd = sessions[i].player1_fd;
                if (sessions[i].player2_fd > max_fd) max_fd = sessions[i].player2_fd;
            }
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &readfds)) {
            struct sockaddr_in cliaddr;
            socklen_t len = sizeof(cliaddr);
            int connfd = accept(sockfd, (SA*)&cliaddr, &len);
            if (connfd < 0) {
                printf("Accept failed...\n");
                continue;
            }
            printf("New client connected (fd: %d)\n", connfd);
            send_to_player(connfd, "SELECT_GAME\n");
            waitingPlayers[numWaiting].connfd = connfd;
            waitingPlayers[numWaiting].gameChoice[0] = '\0';
            numWaiting++;
        }

        for (int i = 0; i < numWaiting; i++) {
            if (FD_ISSET(waitingPlayers[i].connfd, &readfds)) {
                char buff[BUFFER_SIZE];
                int n = read(waitingPlayers[i].connfd, buff, sizeof(buff));
                if (n <= 0) {
                    close(waitingPlayers[i].connfd);
                    for (int j = i; j < numWaiting - 1; j++) waitingPlayers[j] = waitingPlayers[j + 1];
                    numWaiting--;
                    continue;
                }
                buff[n] = '\0';
                if (strncmp(buff, "GAME:", 5) == 0) {
                    strncpy(waitingPlayers[i].gameChoice, buff + 5, 20);
                    waitingPlayers[i].gameChoice[strcspn(waitingPlayers[i].gameChoice, "\n")] = '\0';
                    printf("Player (fd: %d) selected game: %s\n", waitingPlayers[i].connfd, waitingPlayers[i].gameChoice);
                    send_to_player(waitingPlayers[i].connfd, "WAITING\n");

                    int matchIndex = -1;
                    for (int j = 0; j < numWaiting; j++) {
                        if (j != i && strcmp(waitingPlayers[i].gameChoice, waitingPlayers[j].gameChoice) == 0) {
                            matchIndex = j;
                            break;
                        }
                    }

                    if (matchIndex != -1) {
                        GameSession *session = &sessions[numSessions];
                        session->player1_fd = waitingPlayers[matchIndex].connfd;
                        session->player2_fd = waitingPlayers[i].connfd;
                        session->gameOver = 0;
                        char start_msg[50];
                        if (strcmp(waitingPlayers[i].gameChoice, "WORDLE") == 0) {
                            session->gameType = WORDLE;
                            session->turn = 1;
                            session->p1Attempts = 0;
                            session->p2Attempts = 0;
                            session->maxAttempts = 5;
                            strcpy(session->secretWord, wordList[rand() % wordListSize]);
                            printf("Starting Wordle game with secret word: %s\n", session->secretWord);
                            snprintf(start_msg, sizeof(start_msg), "START:WORDLE\n");
                        } else if (strcmp(waitingPlayers[i].gameChoice, "CHESS") == 0) {
                            session->gameType = CHESS;
                            snprintf(start_msg, sizeof(start_msg), "START:CHESS\n");
                        } else if (strcmp(waitingPlayers[i].gameChoice, "SNAKE_LADDER") == 0) {
                            session->gameType = SNAKE_LADDER;
                            snprintf(start_msg, sizeof(start_msg), "START:SNAKE_LADDER\n");
                        } else if (strcmp(waitingPlayers[i].gameChoice, "TIC_TAC_TOE") == 0) {
                            session->gameType = TIC_TAC_TOE;
                            snprintf(start_msg, sizeof(start_msg), "START:TIC_TAC_TOE\n");
                        } else if (strcmp(waitingPlayers[i].gameChoice, "ROCK_PAPER_SCISSOR") == 0) {
                            session->gameType = ROCK_PAPER_SCISSOR;
                            snprintf(start_msg, sizeof(start_msg), "START:ROCK_PAPER_SCISSOR\n");
                        }
                        send_to_player(session->player1_fd, start_msg);
                        send_to_player(session->player1_fd, "Connected as Player 1. Game starting...\n");
                        send_to_player(session->player2_fd, start_msg);
                        send_to_player(session->player2_fd, "Connected as Player 2. Game starting...\n");
                        numSessions++;

                        for (int j = matchIndex; j < numWaiting - 1; j++) waitingPlayers[j] = waitingPlayers[j + 1];
                        numWaiting--;
                        for (int j = i - 1; j < numWaiting - 1; j++) waitingPlayers[j] = waitingPlayers[j + 1];
                        numWaiting--;
                        i--;

                        if (session->gameType == WORDLE) runWordleGame(session);
                        else if (session->gameType == CHESS) runChessGame(session);
                        else if (session->gameType == SNAKE_LADDER) runSnakeLadderGame(session);
                        else if (session->gameType == TIC_TAC_TOE) runTicTacToeGame(session);
                        else if (session->gameType == ROCK_PAPER_SCISSOR) runRockPaperScissorGame(session);
                    }
                }
            }
        }
    }
    close(sockfd);
    return 0;
}
