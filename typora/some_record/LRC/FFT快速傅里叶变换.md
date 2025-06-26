# 快速傅里叶变换



## 📝 FFT核心代码解析（基于ARM CMSIS-DSP库）

[TOC]

### 1️⃣ 复数数组生成函数

c



```c
/**
 * @brief 将两组ADC数据转换为复数格式（实部为采样值，虚部为0）
 * @param[in] adc_data_v 电压信号的ADC采样数组（uint16_t格式）
 * @param[in] adc_data_i 电流信号的ADC采样数组（uint16_t格式）
 */
static void Create_fft_cmp(uint16_t *adc_data_v, uint16_t *adc_data_i)
{
    float *fft_cmp_ptr = fft_cmp;    // 指向电压信号复数数组的指针
    float *fft_cmp2_ptr = fft_cmp2;  // 指向电流信号复数数组的指针

    for (int i = 0; i < 128; i++) {
        // 电压信号：实部为ADC值，虚部初始化为0
        *fft_cmp_ptr++ = (float)adc_data_v[i]; // 实部填充
        *fft_cmp_ptr++ = 0;                    // 虚部清零
      
        // 电流信号：实部为ADC值，虚部初始化为0
        *fft_cmp2_ptr++ = (float)adc_data_i[i];
        *fft_cmp2_ptr++ = 0;
    }
}
```

------

### 2️⃣ FFT计算主函数

c



```c
/**
 * @brief 执行FFT计算并获取频域幅值
 * 功能流程：
 * 1. 准备复数输入 -> 2. 计算时域最大值 -> 3. 执行FFT -> 4. 计算频域幅值
 */
static void fft_count()
{
    uint32_t max_index_v, max_index_i; // 存储时域最大值的位置索引（未使用）

    // 步骤1: 将ADC数据转换为复数格式（实部+虚部）
    Create_fft_cmp(count_val_v, count_val_i);

    // 步骤2: 计算时域信号的最大值（用于后续分析）
    arm_max_f32(fft_cmp,  256, &(count_value_cnt.max_v), &max_index_v); // 电压信号最大值
    arm_max_f32(fft_cmp2, 256, &(count_value_cnt.max_i), &max_index_i); // 电流信号最大值

    // 步骤3: 执行FFT变换（使用预定义的128点FFT配置）
    arm_cfft_f32(&arm_cfft_sR_f32_len128, fft_cmp,  0 /*逆变换标志位*/, 1 /*位反转使能*/);
    arm_cfft_f32(&arm_cfft_sR_f32_len128, fft_cmp2, 0, 1);

    // 步骤4: 计算复数频谱的模值（得到频域幅值）
    arm_cmplx_mag_f32(fft_cmp,  fft_out,  128); // 电压频谱幅值存入fft_out
    arm_cmplx_mag_f32(fft_cmp2, fft_out2, 128); // 电流频谱幅值存入fft_out2
}
```

------

## 🔍 关键点说明

### 📌 函数与参数

| 函数/参数                | 说明                                                  |
| ------------------------ | ----------------------------------------------------- |
| `arm_cfft_f32()`         | ARM库FFT函数，参数1为预定义的FFT配置，参数2为输入数组 |
| `arm_cmplx_mag_f32()`    | 计算复数数组的模值，输出为实数数组（幅值）            |
| `arm_max_f32()`          | 寻找数组中的最大值及其位置索引                        |
| `arm_cfft_sR_f32_len128` | 预生成的128点FFT配置结构体（需提前通过ARM库初始化）   |

------

### 📊 数据结构

c



```c
// 假设已定义的全局数组（需在外部声明）：
float fft_cmp[256];   // 电压信号复数数组（128复数点 = 256浮点数）
float fft_cmp2[256];  // 电流信号复数数组
float fft_out[128];   // 电压频谱幅值输出（实数）
float fft_out2[128];  // 电流频谱幅值输出
```

------

## ⚠️ 注意事项

1. **依赖ARM CMSIS-DSP库**：需在工程中包含ARM数学库（`arm_math.h`）并初始化FFT配置。
2. **数据长度对齐**：输入数组长度必须为FFT点数（此处128点）的2倍（复数存储）。
3. **虚部初始化**：若信号无相位信息，虚部设为0；若需加窗处理，需在此函数中添加窗函数。
4. **结果存储**：频域幅值结果保存在`fft_out`和`fft_out2`数组中，索引对应频率分量的幅值。

 