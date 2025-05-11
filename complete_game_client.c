#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8081
#define MAX 256
#define BUFFER_SIZE 2048
#define SA struct sockaddr

int read_line(int sockfd, char *buf, int size) {
    int n = 0;
    while (n < size - 1) {
        char c;
        int bytes = read(sockfd, &c, 1);
        if (bytes <= 0) return bytes;
        if (c == '\n') break;
        buf[n++] = c;
    }
    buf[n] = '\0';
    return n;
}

void display_sl_board(char *board_msg) {
    printf("\n\033[1mSnake and Ladder Board\033[0m\n");
    printf("┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐\n");
    char *token = strtok(board_msg + 6, ",");
    int pos = 0;
    int row = 10;
    while (token != NULL) {
        int num = atoi(token);
        char marker = ' ';
        if (strlen(token) > 1 && token[strlen(token) - 1] == 'S') marker = 'S';
        if (strlen(token) > 1 && token[strlen(token) - 1] == 'L') marker = 'L';
        if (pos % 10 == 0 && pos > 0) {
            printf("│\n├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤\n");
            row--;
        }
        if (marker == 'S') printf("│\033[31m %2d🐍\033[0m", num);
        else if (marker == 'L') printf("│\033[32m %2d🪜\033[0m", num);
        else printf("│ %2d  ", num);
        pos++;
        token = strtok(NULL, ",");
    }
    printf("│\n└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘\n");
    fflush(stdout); // Ensure board is rendered immediately
}

void playWordle(int sockfd) {
    char buff[MAX];
    int n;
    while (1) {
        bzero(buff, MAX);
        read(sockfd, buff, sizeof(buff));
        printf("%s", buff);
        fflush(stdout); // Ensure message is displayed immediately
        if (strstr(buff, "Game over") || strstr(buff, "wins!") || strstr(buff, "disconnected")) {
            break;
        }
        if (strstr(buff, "Enter a 5-letter guess")) {
            bzero(buff, MAX);
            n = 0;
            printf("Your guess: ");
            fflush(stdout); // Ensure prompt is displayed before input
            while ((buff[n++] = getchar()) != '\n');
            buff[n-1] = '\0';
            write(sockfd, buff, sizeof(buff));
            if (strncmp(buff, "exit", 4) == 0) {
                printf("Client exiting...\n");
                fflush(stdout);
                break;
            }
        }
    }
}

void playChess(int sockfd) {
    char buffer[BUFFER_SIZE];
    while (1) {
        bzero(buffer, BUFFER_SIZE);
        int n = read(sockfd, buffer, BUFFER_SIZE);
        if (n <= 0) {
            printf("\033[1;31mServer disconnected\033[0m\n");
            fflush(stdout); // Ensure disconnection message is displayed
            break;
        }
        buffer[n] = '\0';
        if (strncmp(buffer, "WELCOME:", 8) == 0) {
            printf("%s\n", buffer);
            fflush(stdout); // Ensure welcome message is displayed
        } else if (strncmp(buffer, "MOVE:", 5) == 0) {
            printf("%s\n", buffer);
            fflush(stdout); // Ensure move message is displayed
        } else if (strncmp(buffer, "BOARD_UPDATE", 12) == 0) {
            continue;
        } else if (strncmp(buffer, "TURN", 4) == 0) {
            printf("\n\033[1;36m♟ Your Turn! ♟\033[0m Enter move (e.g., 'P1 e5', 'K1B c6'): ");
            fflush(stdout); // Ensure turn prompt is displayed immediately
            char move[10];
            fgets(move, sizeof(move), stdin);
            move[strcspn(move, "\n")] = '\0';
            char cmd[20];
            snprintf(cmd, sizeof(cmd), "MOVE:%s\n", move);
            write(sockfd, cmd, strlen(cmd));
        } else if (strncmp(buffer, "WINNER:", 7) == 0) {
            printf("\n\033[1;32m🏆 %s 🏆\033[0m\n", buffer + 7);
            fflush(stdout); // Ensure winner message is displayed
            break;
        } else if (strncmp(buffer, "Invalid", 7) == 0) {
            printf("%s\n", buffer);
            fflush(stdout); // Ensure invalid move message is displayed
        } else {
            printf("%s", buffer);
            fflush(stdout); // Ensure board or other messages are displayed
        }
    }
}

void playSnakeLadder(int sockfd) {
    char buff[BUFFER_SIZE];
    int player_id = -1;
    while (1) {
        int n = read_line(sockfd, buff, BUFFER_SIZE);
        if (n <= 0) {
            printf("Server disconnected\n");
            fflush(stdout); // Ensure disconnection message is displayed
            break;
        }
        if (strncmp(buff, "WELCOME:", 8) == 0) {
            sscanf(buff + 8, "Player %d", &player_id);
            printf("You are Player %d\n", player_id);
            fflush(stdout); // Ensure player ID is displayed
        } else if (strncmp(buff, "BOARD:", 6) == 0) {
            display_sl_board(buff);
        } else if (strncmp(buff, "GAME_START", 10) == 0) {
            printf("\n\033[1;33mGame Started!\033[0m\n");
            fflush(stdout); // Ensure game start message is displayed
        } else if (strncmp(buff, "NEW_PLAYER:", 11) == 0) {
            printf("%s", buff + 11);
            fflush(stdout); // Ensure new player message is displayed
        } else if (strncmp(buff, "TURN", 4) == 0) {
            printf("\n\033[1;36mYour Turn!\033[0m Enter 'roll' to roll the dice: ");
            fflush(stdout); // Ensure turn prompt is displayed immediately
            char input[10];
            fgets(input, 10, stdin);
            if (strncmp(input, "roll", 4) == 0) write(sockfd, "ROLL\n", 5);
        } else if (strncmp(buff, "ROLLED:", 7) == 0) {
            int p_id, roll;
            sscanf(buff + 7, "P%d=%d", &p_id, &roll);
            printf("Player %d rolled a %d\n", p_id, roll);
            fflush(stdout); // Ensure roll message is displayed
        } else if (strncmp(buff, "SNAKE:", 6) == 0) {
            int p_id, from, to;
            sscanf(buff + 6, "P%d=%d-%d", &p_id, &from, &to);
            printf("\033[31mPlayer %d slid down a snake from %d to %d\033[0m\n", p_id, from, to);
            fflush(stdout); // Ensure snake message is displayed
        } else if (strncmp(buff, "LADDER:", 7) == 0) {
            int p_id, from, to;
            sscanf(buff + 7, "P%d=%d-%d", &p_id, &from, &to);
            printf("\033[32mPlayer %d climbed a ladder from %d to %d\033[0m\n", p_id, from, to);
            fflush(stdout); // Ensure ladder message is displayed
        } else if (strncmp(buff, "POSITIONS:", 10) == 0) {
            printf("\n\033[1mPlayer Positions:\033[0m\n");
            char *token = strtok(buff + 10, ",");
            while (token != NULL) {
                int p_id, pos;
                sscanf(token, "P%d=%d", &p_id, &pos);
                printf("Player \033[1m%d\033[0m: %d\n", p_id, pos);
                token = strtok(NULL, ",");
            }
            fflush(stdout); // Ensure positions are displayed
        } else if (strncmp(buff, "WINNER:", 7) == 0) {
            printf("\n\033[1;32m%s wins!\033[0m\n", buff + 7);
            fflush(stdout); // Ensure winner message is displayed
            break;
        }
    }
}

void playTicTacToe(int sockfd) {
    char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(sockfd, buffer, sizeof(buffer));
        if (n <= 0) {
            printf("Server disconnected\n");
            fflush(stdout); // Ensure disconnection message is displayed
            break;
        }
        printf("%s", buffer);
        fflush(stdout); // Ensure board or message is displayed
        if (strstr(buffer, "Your turn")) {
            memset(buffer, 0, sizeof(buffer));
            printf("Enter row and col (0-2 0-2): ");
            fflush(stdout); // Ensure prompt is displayed before input
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            write(sockfd, buffer, strlen(buffer));
        }
    }
}

void playRockPaperScissor(int sockfd) {
    char buff[MAX];
    int n;
    while (1) {
        bzero(buff, MAX);
        read(sockfd, buff, sizeof(buff));
        printf("%s", buff);
        fflush(stdout); // Ensure message is displayed
        if (strstr(buff, "Game over") || strstr(buff, "wins the game")) {
            break;
        }
        if (strstr(buff, "Enter STONE") || strstr(buff, "Enter PAPER") || strstr(buff, "Enter SCISSORS")) {
            bzero(buff, MAX);
            n = 0;
            printf("Your move: ");
            fflush(stdout); // Ensure prompt is displayed before input
            while ((buff[n++] = getchar()) != '\n');
            buff[n - 1] = '\0';
            write(sockfd, buff, sizeof(buff));
            if (strncmp(buff, "exit", 4) == 0) {
                printf("Client exiting...\n");
                fflush(stdout); // Ensure exit message is displayed
                break;
            }
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[BUFFER_SIZE];
    int game_selected = 0;
    char *game_name = NULL;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed...\n");
        fflush(stdout); // Ensure error message is displayed
        exit(0);
    }
    printf("\033[1;34mSocket successfully created.\033[0m\n");
    fflush(stdout); // Ensure success message is displayed

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("172.30.0.248");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("\033[1;31mConnection to server failed...\033[0m\n");
        fflush(stdout); // Ensure error message is displayed
        exit(0);
    }
    printf("\033[1;32mConnected to server!\033[0m\n");
    fflush(stdout); // Ensure success message is displayed

    printf("\n\033[1;36m=====================================\033[0m\n");
    printf("\033[1;33m    Welcome to Game Studios! 🎮    \033[0m\n");
    printf("\033[1;36m=====================================\033[0m\n");
    printf("\n\033[1mSelect a Game to Play:\033[0m\n");
    printf("  \033[1;34m1.\033[0m Wordle\n");
    printf("  \033[1;34m2.\033[0m Chess\n");
    printf("  \033[1;34m3.\033[0m Snake and Ladder\n");
    printf("  \033[1;34m4.\033[0m Tic Tac Toe\n");
    printf("  \033[1;34m5.\033[0m Rock Paper Scissors\n");
    printf("\n\033[1mEnter your choice (1-5):\033[0m ");
    fflush(stdout); // Ensure welcome UI and prompt are displayed

    char choice[10];
    fgets(choice, sizeof(choice), stdin);
    choice[strcspn(choice, "\n")] = '\0';
    int game_choice = atoi(choice);

    switch (game_choice) {
        case 1: game_name = "WORDLE"; break;
        case 2: game_name = "CHESS"; break;
        case 3: game_name = "SNAKE_LADDER"; break;
        case 4: game_name = "TIC_TAC_TOE"; break;
        case 5: game_name = "ROCK_PAPER_SCISSOR"; break;
        default:
            printf("\033[1;31mInvalid choice! Exiting.\033[0m\n");
            fflush(stdout); // Ensure error message is displayed
            close(sockfd);
            return 0;
    }

    char cmd[30];
    snprintf(cmd, sizeof(cmd), "GAME:%s\n", game_name);
    write(sockfd, cmd, strlen(cmd));
    printf("\n\033[1;33mWaiting for another player to join %s...\033[0m\n", game_name);
    fflush(stdout); // Ensure waiting message is displayed

    while (!game_selected) {
        int n = read_line(sockfd, buffer, BUFFER_SIZE);
        if (n <= 0) {
            printf("\033[1;31mServer disconnected\033[0m\n");
            fflush(stdout); // Ensure disconnection message is displayed
            break;
        }
        if (strncmp(buffer, "WAITING", 7) == 0) {
            printf(".");
            // No fflush here to avoid excessive flushing in loop
            sleep(1);
            continue;
        } else if (strncmp(buffer, "START:", 6) == 0) {
            char selected_game[20];
            sscanf(buffer + 6, "%s", selected_game);
            if (strcmp(selected_game, game_name) == 0) {
                game_selected = 1;
            }
        } else if (strncmp(buffer, "Connected as Player", 19) == 0) {
            printf("\n%s", buffer);
            fflush(stdout); // Ensure player connection message is displayed
        } else if (strncmp(buffer, "ERROR:", 6) == 0) {
            printf("\n\033[1;31m%s\033[0m\n", buffer + 6);
            fflush(stdout); // Ensure error message is displayed
            break;
        }
    }

    if (game_selected) {
        if (strcmp(game_name, "WORDLE") == 0) playWordle(sockfd);
        else if (strcmp(game_name, "CHESS") == 0) playChess(sockfd);
        else if (strcmp(game_name, "SNAKE_LADDER") == 0) playSnakeLadder(sockfd);
        else if (strcmp(game_name, "TIC_TAC_TOE") == 0) playTicTacToe(sockfd);
        else if (strcmp(game_name, "ROCK_PAPER_SCISSOR") == 0) playRockPaperScissor(sockfd);
    }

    close(sockfd);
    printf("\033[1;34mDisconnected from server.\033[0m\n");
    fflush(stdout); // Ensure disconnection message is displayed
    return 0;
}
