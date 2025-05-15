# Multiplayer Game Server Documentation

## Overview
This project implements a multiplayer game server and client in C, allowing two players to connect and play one of five games: Chess, Wordle, Snake and Ladder, Tic-Tac-Toe, or Rock Paper Scissors. The server manages game sessions, pairs players, and handles game logic, while the client provides a user interface for players to select and play games.

### Key Features
- Supports five multiplayer games.
- TCP-based server-client communication.
- Concurrent game sessions using `select` for I/O multiplexing.
- ANSI-colored terminal output for enhanced user experience.
- Robust error handling for disconnections and invalid inputs.

## Code Structure

### Files
- **game_server.c**: Implements the server, handling client connections, game session management, and game-specific logic.
- **game_client.c**: Implements the client, providing a menu for game selection and game-specific interfaces.

### Key Components
#### game_server.c
- **Data Structures**:
  - `Piece` and `ChessBoard`: Represent chess pieces and the 8x8 board.
  - `SnakeLadder`: Defines snakes and ladders for the Snake and Ladder game.
  - `WaitingPlayer`: Stores client file descriptors and game choices for unpaired players.
  - `GameSession`: Manages a game session, including player file descriptors, game type, and game-specific state (e.g., chess board, Wordle secret word).
- **Core Functions**:
  - `main`: Sets up the TCP server, accepts connections, and uses `select` to handle multiple clients.
  - `run[Game]Game`: Game-specific logic (e.g., `runChessGame`, `runWordleGame`).
  - `send_to_player` and `broadcast`: Handle message sending to one or both players.
  - Game-specific helpers (e.g., `move_piece` for Chess, `checkGuess` for Wordle).
- **Key Logic**:
  - Listens on port 8081.
  - Matches players with the same game choice into a `GameSession`.
  - Manages turn-based gameplay and validates moves.
  - Sends game state updates (e.g., board, scores) to clients.

#### game_client.c
- **Data Structures**:
  - Minimal; primarily uses buffers for communication.
- **Core Functions**:
  - `main`: Connects to the server, displays a game selection menu, and routes to the appropriate game function.
  - `play[Game]`: Game-specific client logic (e.g., `playChess`, `playWordle`).
  - `read_line`: Reads server messages terminated by newline.
  - Game-specific display functions (e.g., `display_sl_board` for Snake and Ladder).
- **Key Logic**:
  - Connects to the server at `127.0.0.1:8081`.
  - Sends the selected game type and waits for a match.
  - Receives and displays game state (e.g., boards, prompts) and sends user inputs.

## Prerequisites
- **Operating System**: Linux/Unix (tested on Ubuntu).
- **Compiler**: GCC.
- **Dependencies**: Standard C libraries (`stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, `sys/socket.h`, `netinet/in.h`, `arpa/inet.h`).
- **Terminal**: Supports ANSI escape codes for colored output.

## How to Upload and Run the Code

### Uploading the Code
1. **Create Files**:
   - On your Linux machine, create two files: `game_server.c` and `game_client.c`.
   - Copy the contents of each file from the provided code (see artifacts in the project context).
   - Save them in a project directory (e.g., `~/game_project/`).

2. **Using SCP (Optional)**:
   - If uploading to a remote server:
     ```bash
     scp game_server.c game_client.c user@remote_host:~/game_project/
     ```
   - Replace `user@remote_host` with your server's credentials and `~/game_project/` with the target directory.

3. **Using Git (Optional)**:
   - Initialize a Git repository:
     ```bash
     mkdir game_project
     cd game_project
     git init
     ```
   - Create `game_server.c` and `game_client.c`, add, and commit:
     ```bash
     git add game_server.c game_client.c
     git commit -m "Initial commit"
     ```
   - Push to a remote repository (e.g., GitHub):
     ```bash
     git remote add origin <repository_url>
     git push origin main
     ```
   - Clone on the target machine:
     ```bash
     git clone <repository_url>
     ```

### Compiling the Code
1. **Navigate to Project Directory**:
   ```bash
   cd ~/game_project
   ```

2. **Compile Server**:
   ```bash
   gcc game_server.c -o game_server
   ```

3. **Compile Client**:
   ```bash
   gcc game_client.c -o game_client
   ```

4. **Verify Executables**:
   - Ensure `game_server` and `game_client` are created in the directory:
     ```bash
     ls
     ```

### Running the Code
1. **Start the Server**:
   - Open a terminal and run:
     ```bash
     ./game_server
     ```
   - Expected output:
     ```
     Socket successfully created..
     Socket successfully bound..
     Server listening..
     ```

2. **Run the First Client**:
   - Open a new terminal in the same directory:
     ```bash
     ./game_client
     ```
   - Expected output:
     ```
     Socket successfully created.
     Connected to server!
     =============================
         Welcome to Game Studios! 🎮
     =============================
     Select a Game to Play:
       1. Wordle
       2. Chess
       3. Snake and Ladder
       4. Tic Tac Toe
       5. Rock Paper Scissors
     Enter your choice (1-5):
     ```
   - Enter a number (e.g., `2` for Chess). The client will wait for another player:
     ```
     Waiting for another player to join CHESS...
     ```

3. **Run the Second Client**:
   - Open another terminal and run:
     ```bash
     ./game_client
     ```
   - Select the same game (e.g., `2` for Chess). Both clients will start the game:
     ```
     Connected as Player 1. Game starting...
     🎉 CHESS GAME STARTED! 🎉
     ✨ CHESS BOARD ✨
     ...
     ```

4. **Playing the Game**:
   - Follow game-specific prompts (e.g., for Chess, enter moves like `P1 e5`).
   - The game continues until a winner is declared or a player disconnects.

5. **Stopping the Server**:
   - Press `Ctrl+C` in the server terminal to stop it.
   - Clients will disconnect automatically.

### Running on Multiple Machines
- **Update Server IP**:
  - In `game_client.c`, change `inet_addr("127.0.0.1")` to the server’s IP address.
  - Recompile the client:
    ```bash
    gcc game_client.c -o game_client
    ```
- **Ensure Port Accessibility**:
  - Ensure port 8081 is open on the server machine (e.g., configure firewall with `ufw allow 8081`).
- **Run Server and Clients**:
  - Start the server on the host machine.
  - Run clients on different machines, connecting to the server’s IP.

## Code Details

### Game-Specific Logic
1. **Chess**:
   - **Server**: Manages an 8x8 board with pieces (`Piece` struct). Validates moves using `is_legal_move` (e.g., pawn moves, knight L-shape). Sends board updates and turn prompts.
   - **Client**: Displays the board with ANSI colors, receives moves, and sends them to the server (format: `MOVE:P1 e5`).
   - **Win Condition**: Capturing the opponent’s king.

2. **Wordle**:
   - **Server**: Selects a random 5-letter word from a predefined list. Checks guesses and provides feedback (e.g., `A****` for correct letters).
   - **Client**: Prompts for 5-letter guesses and displays feedback.
   - **Win Condition**: Guessing the word within 5 attempts or game over after both players exhaust attempts.

3. **Snake and Ladder**:
   - **Server**: Manages a 100-square board with predefined snakes and ladders. Handles dice rolls and position updates.
   - **Client**: Displays the board with snakes (`🐍`) and ladders (`🪜`), prompts for `roll`.
   - **Win Condition**: First player to reach or exceed position 100.

4. **Tic-Tac-Toe**:
   - **Server**: Maintains a 3x3 board, validates moves, and checks for wins or draws.
   - **Client**: Displays the board and prompts for row/column inputs (e.g., `0 1`).
   - **Win Condition**: Three in a row/column/diagonal or a draw if the board is full.

5. **Rock Paper Scissors**:
   - **Server**: Manages best-of-3 rounds, compares moves, and tracks scores.
   - **Client**: Prompts for `STONE`, `PAPER`, or `SCISSORS`.
   - **Win Condition**: First to win 2 rounds.

### Communication Protocol
- **Messages**:
  - Server to Client:
    - `START:[GAME]`: Game begins.
    - `BOARD_UPDATE`, `BOARD:[data]`, `TURN`: Game state updates.
    - `WINNER:[Player]`: Game over with winner.
    - `ERROR:[Message]`: Invalid input or state.
  - Client to Server:
    - `GAME:[GameName]`: Game selection.
    - `MOVE:[Move]`, `ROLL`: Player actions.
- **Format**: Messages are newline-terminated strings for reliable parsing.

### Error Handling
- **Server**:
  - Handles client disconnections by ending the session and notifying the other player.
  - Validates inputs (e.g., move formats, guess lengths).
- **Client**:
  - Displays server errors and exits on disconnection.
  - Uses `fflush(stdout)` and `setvbuf(stdout, NULL, _IONBF, 0)` to prevent output buffering issues.

### Limitations
- Supports only two players per game session.
- No persistent game state (games end on disconnection).
- Chess lacks advanced rules (e.g., castling, en passant).
- Limited error recovery for network issues.

## Troubleshooting
1. **Board Not Displaying (Chess)**:
   - Ensure terminal supports ANSI colors.
   - Check `[DEBUG]` logs in the client for received messages.
   - Verify `BUFFER_SIZE` (2048) is sufficient; increase to 4096 if truncated.
   - Run `stty -icanon min 1 time 0` to disable terminal buffering.

2. **Connection Issues**:
   - Confirm server is running and port 8081 is open (`netstat -tuln | grep 8081`).
   - Check client IP address matches server’s.

3. **Delayed Output**:
   - Verify `setvbuf(stdout, NULL, _IONBF, 0)` in `game_client.c`.
   - Test in a different terminal (e.g., `xterm` instead of VS Code).

4. **Compilation Errors**:
   - Ensure GCC is installed (`sudo apt install gcc`).
   - Check for missing headers or syntax errors in the code.

## Future Improvements
- Add support for more players per game.
- Implement advanced Chess rules.
- Add a graphical interface using a library like SDL.
- Include game state persistence for reconnection.
- Enhance security with input sanitization and encryption.

## Contact
For issues or contributions, refer to the project repository (if hosted) or contact the developer.

---
*Generated on May 15, 2025*
