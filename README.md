**Multiplayer Game Server**

This project is a C-based multiplayer game server and client application that supports five interactive games: Chess, Wordle, Snake and Ladder, Tic Tac Toe, and Rock Paper Scissors. Built using socket programming, the server handles multiple clients concurrently, matching players for two-player game sessions over a TCP connection. Key features include:

- **Games**: Turn-based implementations of Chess (with move validation), Wordle (5-letter word guessing), Snake and Ladder (with snakes and ladders mechanics), Tic Tac Toe (3x3 grid), and Rock Paper Scissors (best-of-n rounds).
- **Networking**: Server uses `select` for asynchronous I/O, supporting multiple simultaneous game sessions. Clients connect via IP and port (default: `127.0.0.1:8081`), with potential for local or internet play with port forwarding.
- **UI**: Client features a colorful console-based interface with ANSI-colored game boards and prompts. Server logs connection and game events.
- **Extensibility**: Modular design allows adding new games by extending the `GameSession` structure and game logic functions.

Ideal for learning socket programming, game development, and client-server architecture. Currently supports two players per game session, with potential for multi-player extensions. Compile and run on Unix-like systems (e.g., Linux, macOS) with a C compiler.

**Usage**:
- Compile: `gcc game_server.c -o game_server` and `gcc game_client.c -o game_client`
- Run server: `./game_server`
- Run client: `./game_client` and select a game (1–5)

**Future Enhancements**:
- Support for multiple players in games like Snake and Ladder.
- Dynamic server IP input for clients.
- Authentication and secure communication.

**Modular Function diagram**
+------------------------------------+
|          game_server.c             |
|------------------------------------|
| main()                             |
|   Initializes TCP server, handles  |
|   client connections using select  |
|                                    |
|   +--> send_to_player()            |
|   |     Sends message to a client  |
|   +--> broadcast()                 |
|   |     Sends message to both      |
|   |     players in a session       |
|   |                                |
|   +--> runWordleGame()             |
|   |     Manages Wordle game logic  |
|   +--> runChessGame()              |
|   |     Manages Chess game logic   |
|   |     +--> init_chess_board()    |
|   |     +--> get_chess_board_string|
|   |     +--> move_piece()          |
|   |     +--> is_legal_move()       |
|   |     +--> check_chess_winner()  |
|   |     +--> send_chess_board()    |
|   +--> runSnakeLadderGame()        |
|   |     Manages Snake & Ladder     |
|   |     +--> send_sl_board()       |
|   |     +--> send_sl_positions()   |
|   +--> runTicTacToeGame()          |
|   |     Manages Tic-Tac-Toe        |
|   |     +--> init_ttt_board()      |
|   |     +--> get_ttt_board_display |
|   |     +--> check_ttt_winner()    |
|   |     +--> is_ttt_draw()         |
|   |     +--> broadcast_ttt_board() |
|   +--> runRockPaperScissorGame()   |
|         Manages Rock Paper Scissors|
|         +--> get_rps_winner()      |
+------------------------------------+
          | TCP Communication |
          v                  ^
+------------------------------------+
|          game_client.c             |
|------------------------------------|
| main()                             |
|   Connects to server, displays     |
|   game selection menu              |
|                                    |
|   +--> read_line()                 |
|   |     Reads newline-terminated   |
|   |     messages from server       |
|   +--> playWordle()                |
|   |     Handles Wordle interface   |
|   +--> playChess()                 |
|   |     Handles Chess interface    |
|   +--> playSnakeLadder()           |
|   |     Handles Snake & Ladder     |
|   |     +--> display_sl_board()    |
|   +--> playTicTacToe()             |
|   |     Handles Tic-Tac-Toe        |
|   +--> playRockPaperScissor()      |
|         Handles Rock Paper Scissors|
+------------------------------------+

**License**: MIT (or specify your preferred license).
