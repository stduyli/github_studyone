#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

jmp_buf jump_buffer;

void test(int x, int y) {
    int *pointer1 = (int *)calloc(5, sizeof(int));
    if (pointer1 == NULL) {
        longjmp(jump_buffer, 2);  // 内存错误跳转
    }

    if (y == 0) {
        free(pointer1);
        longjmp(jump_buffer, 1);  // 除零错误跳转
    }

    printf("Result: %f\n", (double)x / (double)y);
    free(pointer1);
    printf("delete pointer1\n");
}

int main() {
    while (1) {
        int x, y;
        printf("input x: ");
        scanf("%d", &x);
        printf("input y: ");
        scanf("%d", &y);

        switch (setjmp(jump_buffer)) {
            case 0:  // 正常执行
                test(x, y);
                break;
            case 1:  // 除零错误
                printf("Error: Division by zero!\n");
                break;
            case 2:  // 内存错误
                printf("Error: Memory allocation failed!\n");
                break;
            default:
                printf("Unknown error!\n");
        }
    }
    return 0;
}