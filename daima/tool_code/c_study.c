#include <stdio.h>
#include <stdlib.h>

#define SIZE 15   // 棋盘大小
#define EMPTY '.' // 表示空位
#define PLAYER1 'X'
#define PLAYER2 'O'

char board[SIZE][SIZE];

// 初始化棋盘
void init_board() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = EMPTY;
        }
    }
}

// 打印棋盘
void print_board() {
    system("cls"); // Windows 清屏，Linux/Mac 可改为 system("clear")

    printf("  ");
    for (int i = 0; i < SIZE; i++) {
        printf("%2d ", i);
    }
    printf("\n");

    for (int i = 0; i < SIZE; i++) {
        printf("%2d ", i);
        for (int j = 0; j < SIZE; j++) {
            printf(" %c ", board[i][j]);
        }
        printf("\n");
    }
}

// 判断是否胜利
int check_win(char player) {
    // 四个方向：横、竖、正斜、反斜
    int dx[] = {0, 1, 1, 1};
    int dy[] = {1, 0, 1, -1};

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == player) {
                for (int dir = 0; dir < 4; dir++) {
                    int count = 1;
                    int x = i;
                    int y = j;

                    while (1) {
                        x += dx[dir];
                        y += dy[dir];
                        if (x >= 0 && x < SIZE && y >= 0 && y < SIZE && board[x][y] == player) {
                            count++;
                        } else {
                            break;
                        }
                    }

                    if (count >= 5) {
                        return 1; // 获胜
                    }
                }
            }
        }
    }
    return 0;
}

// 主游戏循环
void play_game() {
    int moves = 0;
    char current_player = PLAYER1;

    init_board();

    while (moves < SIZE * SIZE) {
        print_board();
        int row, col;

        printf("玩家 %c 的回合，请输入坐标（行 列）: ", current_player);
        scanf("%d %d", &row, &col);

        if (row < 0 || row >= SIZE || col < 0 || col >= SIZE) {
            printf("坐标无效，请重新输入。\n");
            continue;
        }

        if (board[row][col] != EMPTY) {
            printf("位置已被占用，请重新输入。\n");
            continue;
        }

        board[row][col] = current_player;
        moves++;

        if (check_win(current_player)) {
            print_board();
            printf("🎉 玩家 %c 获胜！\n", current_player);
            return;
        }

        current_player = (current_player == PLAYER1) ? PLAYER2 : PLAYER1;
    }

    print_board();
    printf("平局！棋盘已满。\n");
}

// 主函数
int main() {
    printf("=== 五子棋游戏 ===\n");
    printf("规则：两个玩家轮流输入坐标（行 列），先连成五子者获胜。\n\n");
    play_game();
    return 0;
}