#include <math.h>
#include <stdio.h>

// 生成DAC正弦波查找表的函数
void generate_sine_lut(unsigned short *lut, int length) 
{
    double phase_increment = 2.0 * 3.1415926535 / length;  // 每个样本的相位增量
    double phase = 0.0;  // 初始相位

    for (int i = 0; i < length; i++) {
        // 计算当前样本的正弦值，并缩放到0到4095的范围
        lut[i] = (unsigned short)((sin(phase) + 1.0) * 2047.5);
        phase += phase_increment;  // 更新相位
    }
}

int main() {
    const int lut_length = 64;  // 查找表的长度
    unsigned short lut[lut_length];  // 查找表数组

    // 生成查找表
    generate_sine_lut(lut, lut_length);

    // 打印查找表的内容（可选）
    for (int i = 0; i < lut_length; i++) {
        printf("%d, ", lut[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }

    return 0;
}