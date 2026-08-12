#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define PIECE_SIZE 4

static struct termios orig_termios;

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int kbhit(void) {
    struct timeval tv = {0, 0};
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0;
}

int read_char(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }
    return -1;
}

const int pieces[7][4][PIECE_SIZE][PIECE_SIZE] = {
    // I
    {
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}
    },
    // J
    {
        {{1,0,0},{1,1,1},{0,0,0},{0,0,0}},
        {{0,1,1},{0,1,0},{0,1,0},{0,0,0}},
        {{0,0,0},{1,1,1},{0,0,1},{0,0,0}},
        {{0,1,0},{0,1,0},{1,1,0},{0,0,0}}
    },
    // L
    {
        {{0,0,1},{1,1,1},{0,0,0},{0,0,0}},
        {{0,1,0},{0,1,0},{0,1,1},{0,0,0}},
        {{0,0,0},{1,1,1},{1,0,0},{0,0,0}},
        {{1,1,0},{0,1,0},{0,1,0},{0,0,0}}
    },
    // O
    {
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}
    },
    // S
    {
        {{0,1,1},{1,1,0},{0,0,0},{0,0,0}},
        {{0,1,0},{0,1,1},{0,0,1},{0,0,0}},
        {{0,0,0},{0,1,1},{1,1,0},{0,0,0}},
        {{1,0,0},{1,1,0},{0,1,0},{0,0,0}}
    },
    // T
    {
        {{0,1,0},{1,1,1},{0,0,0},{0,0,0}},
        {{0,1,0},{0,1,1},{0,1,0},{0,0,0}},
        {{0,0,0},{1,1,1},{0,1,0},{0,0,0}},
        {{0,1,0},{1,1,0},{0,1,0},{0,0,0}}
    },
    // Z
    {
        {{1,1,0},{0,1,1},{0,0,0},{0,0,0}},
        {{0,0,1},{0,1,1},{0,1,0},{0,0,0}},
        {{0,0,0},{1,1,0},{0,1,1},{0,0,0}},
        {{0,1,0},{1,1,0},{1,0,0},{0,0,0}}
    }
};

int board[BOARD_HEIGHT][BOARD_WIDTH];

int piece_type = 0;
int piece_rotation = 0;
int piece_x = 3;
int piece_y = 0;
int score = 0;

int collision(int nx, int ny, int rotation) {
    for (int py = 0; py < PIECE_SIZE; py++) {
        for (int px = 0; px < PIECE_SIZE; px++) {
            int cell = pieces[piece_type][rotation][py][px];
            if (!cell) continue;
            int x = nx + px;
            int y = ny + py;
            if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT) return 1;
            if (board[y][x]) return 1;
        }
    }
    return 0;
}

void place_piece(void) {
    for (int py = 0; py < PIECE_SIZE; py++) {
        for (int px = 0; px < PIECE_SIZE; px++) {
            if (pieces[piece_type][piece_rotation][py][px]) {
                board[piece_y + py][piece_x + px] = 1;
            }
        }
    }
}

void clear_lines(void) {
    int lines = 0;
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (!board[y][x]) { full = 0; break; }
        }
        if (full) {
            lines++;
            for (int sy = y; sy > 0; sy--) {
                memcpy(board[sy], board[sy - 1], sizeof(board[0]));
            }
            memset(board[0], 0, sizeof(board[0]));
            y++;
        }
    }
    score += lines * 100;
}

void spawn_piece(void) {
    piece_type = rand() % 7;
    piece_rotation = 0;
    piece_x = 3;
    piece_y = 0;
    if (collision(piece_x, piece_y, piece_rotation)) {
        printf("Game Over! Final score: %d\n", score);
        disable_raw_mode();
        exit(0);
    }
}

void draw(void) {
    printf("\x1b[H\x1b[2J");
    printf("Score: %d\n", score);
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        printf("|");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int filled = board[y][x];
            for (int py = 0; py < PIECE_SIZE; py++) {
                for (int px = 0; px < PIECE_SIZE; px++) {
                    int piece_cell = 0;
                    int cell_x = piece_x + px;
                    int cell_y = piece_y + py;
                    if (cell_x == x && cell_y == y) {
                        piece_cell = pieces[piece_type][piece_rotation][py][px];
                    }
                    if (piece_cell) filled = 2;
                }
            }
            if (filled == 2) {
                printf("[]");
            } else if (filled) {
                printf("##");
            } else {
                printf("  ");
            }
        }
        printf("|\n");
    }
    printf("+");
    for (int i = 0; i < BOARD_WIDTH * 2; i++) printf("-");
    printf("+\n");
    printf("Controls: a/left, d/right, s/down, w/rotate, space/drop, q/quit\n");
}

int main(void) {
    srand((unsigned int)time(NULL));
    enable_raw_mode();
    memset(board, 0, sizeof(board));
    spawn_piece();
    draw();

    struct timeval last_tick;
    gettimeofday(&last_tick, NULL);
    const int tick_ms = 500;

    while (1) {
        if (kbhit()) {
            int c = read_char();
            if (c == 'q') break;
            if (c == 'a' || c == 'A') {
                if (!collision(piece_x - 1, piece_y, piece_rotation)) piece_x--;
            } else if (c == 'd' || c == 'D') {
                if (!collision(piece_x + 1, piece_y, piece_rotation)) piece_x++;
            } else if (c == 's' || c == 'S') {
                if (!collision(piece_x, piece_y + 1, piece_rotation)) piece_y++;
            } else if (c == 'w' || c == 'W') {
                int next_rot = (piece_rotation + 1) % 4;
                if (!collision(piece_x, piece_y, next_rot)) piece_rotation = next_rot;
            } else if (c == ' ') {
                while (!collision(piece_x, piece_y + 1, piece_rotation)) {
                    piece_y++;
                }
            }
            draw();
        }

        struct timeval now;
        gettimeofday(&now, NULL);
        long elapsed = (now.tv_sec - last_tick.tv_sec) * 1000 + (now.tv_usec - last_tick.tv_usec) / 1000;
        if (elapsed >= tick_ms) {
            last_tick = now;
            if (!collision(piece_x, piece_y + 1, piece_rotation)) {
                piece_y++;
            } else {
                place_piece();
                clear_lines();
                spawn_piece();
            }
            draw();
        }

        usleep(10000);
    }

    disable_raw_mode();
    printf("游戏结束。最终得分: %d\n", score);
    return 0;
}
